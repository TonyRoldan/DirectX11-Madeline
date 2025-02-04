#include "Model.h"
#include <SimpleMath.h>
#include <queue>

using namespace MAD;

MAD::Model::Model()
{
	parser = new Assimp::Importer();

	globalInverse = GW::MATH::GIdentityMatrixF;
	world = GW::MATH::GIdentityMatrixF;

	positions.clear();
	uvs.clear();
	norms.clear();
	indices.clear();

	meshes.clear();
	vertices.clear();
	indices.clear();

	model = nullptr;
	skeleton = nullptr;
	vertexStart = 0;
	indexStart = 0;
	materialStart = 0;
}

MAD::Model::~Model()
{

}

MAD::Model::Model(const Model& other)
	: parser(new Assimp::Importer()), skeleton(other.skeleton),
	positions(other.positions), uvs(other.uvs), norms(other.norms),
	bones(other.bones), boneMap(other.boneMap), boneProps(other.boneProps),
	globalInverse(other.globalInverse), model(other.model),
	modelName(other.modelName), meshes(other.meshes), vertices(other.vertices),
	indices(other.indices), vertexStart(other.vertexStart), indexStart(other.indexStart), 
	materials(other.materials), materialStart(other.materialStart), 
	currPose(other.currPose), skeletonVerts(other.skeletonVerts), world(other.world){}

Model& MAD::Model::operator=(const Model& other)
{
	if (this == &other) {
		return *this;
	}

	delete parser;

	parser = new Assimp::Importer();
	skeleton = other.skeleton;
	positions = other.positions;
	uvs = other.uvs;
	norms = other.norms;
	bones = other.bones;
	boneMap = other.boneMap;
	boneProps = other.boneProps;
	globalInverse = other.globalInverse;
	model = other.model;
	modelName = other.modelName;
	meshes = other.meshes;
	vertices = other.vertices;
	indices = other.indices;
	vertexStart = other.vertexStart;
	indexStart = other.indexStart;
	materials = other.materials;
	materialStart = other.materialStart;
	currPose = other.currPose;
	skeletonVerts = other.skeletonVerts;
	world = other.world;

	return *this;
}

bool MAD::Model::LoadModel(const std::string& filePath, const std::string& fbxName)
{
	std::string fullPath = filePath + fbxName;
	model = parser->ReadFile(fullPath, ASSIMP_FLAGS);

	if (model)
	{
		modelName = fbxName;
		skeleton = FindSkeletonNode(model);
		ParseModel(model);
		globalInverse = (GW::MATH::GMATRIXF&)(model->mRootNode->mTransformation);
		GW::MATH::GMatrix::InverseF(globalInverse, globalInverse);
		return true;
	}

	return false;
}

void MAD::Model::UpdatePose(float duration, unsigned animationNdx)
{
	GW::GReturn ret = {};

	skeletonVerts.clear();
	currPose.clear();

	currPose = BoneTransform(duration, animationNdx);

	for (size_t i = 0; i < boneProps.size(); i++)
	{
		if (boneProps[i].parentNdx == -1 ||   // if the current bone has no parent, ignore
			boneProps[i].parentName == "Bone")  // specialized case. 
			continue;


		const BoneProperties& current_bone = boneProps[i];
		const BoneProperties& parent_bone = boneProps[boneProps[i].parentNdx];

		JointVertex v1 = JointVertex();
		JointVertex v2 = JointVertex();

		GW::MATH::GMATRIXF current_bone_matrix = current_bone.skeletonMatrix;
		GW::MATH::GMATRIXF parent_bone_matrix = parent_bone.skeletonMatrix;

		ret = GW::MATH::GMatrix::TransposeF(current_bone_matrix, current_bone_matrix);
		ret = GW::MATH::GMatrix::TransposeF(parent_bone_matrix, parent_bone_matrix);

		v1.pos = (GW::MATH2D::GVECTOR3F&)current_bone_matrix.row4.xyz();
		v2.pos = (GW::MATH2D::GVECTOR3F&)parent_bone_matrix.row4.xyz();

		skeletonVerts.push_back(v1);
		skeletonVerts.push_back(v2);
	}
}

void MAD::Model::UpdatePoseBlended(float duration, unsigned startAnimationNdx, unsigned endAnimationNdx, float transitionTime)
{
	GW::GReturn ret = {};

	skeletonVerts.clear();
	currPose.clear();

	currPose = BoneTransformBlended(duration, startAnimationNdx, endAnimationNdx, transitionTime);

	for (size_t i = 0; i < boneProps.size(); i++)
	{
		if (boneProps[i].parentNdx == -1 ||   // if the current bone has no parent, ignore
			boneProps[i].parentName == "Bone")  // specialized case. 
			continue;


		const BoneProperties& current_bone = boneProps[i];
		const BoneProperties& parent_bone = boneProps[boneProps[i].parentNdx];

		JointVertex v1 = JointVertex();
		JointVertex v2 = JointVertex();

		GW::MATH::GMATRIXF current_bone_matrix = current_bone.skeletonMatrix;
		GW::MATH::GMATRIXF parent_bone_matrix = parent_bone.skeletonMatrix;

		ret = GW::MATH::GMatrix::TransposeF(current_bone_matrix, current_bone_matrix);
		ret = GW::MATH::GMatrix::TransposeF(parent_bone_matrix, parent_bone_matrix);

		v1.pos = (GW::MATH2D::GVECTOR3F&)current_bone_matrix.row4.xyz();
		v2.pos = (GW::MATH2D::GVECTOR3F&)parent_bone_matrix.row4.xyz();

		skeletonVerts.push_back(v1);
		skeletonVerts.push_back(v2);
	}
}


void MAD::Model::ParseModel(const aiScene* model)
{
	meshes.resize(model->mNumMeshes);

	unsigned numVerts = 0;
	unsigned numIndices = 0;

	for (int i = 0; i < model->mNumMeshes; i++)
	{
		const aiMesh* currMesh = model->mMeshes[i];

		meshes[i].indexCount = currMesh->mNumFaces * 3;
		meshes[i].vertexStart = numVerts;
		meshes[i].indexStart = numIndices;
		meshes[i].materialStart = currMesh->mMaterialIndex;

		numVerts += currMesh->mNumVertices;
		numIndices += meshes[i].indexCount;
	}

	bones.resize(numVerts);

	for (int i = 0; i < model->mNumMaterials; i++)
	{
		if (model->HasMaterials())
		{
			Material* mat = new Material();
			model->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, mat->attrib.diffReflect);
			model->mMaterials[i]->Get(AI_MATKEY_OPACITY, mat->attrib.dissolve);
			model->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, mat->attrib.specReflect);
			model->mMaterials[i]->Get(AI_MATKEY_SHININESS, mat->attrib.spec);
			model->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, mat->attrib.ambReflect);
			model->mMaterials[i]->Get(AI_MATKEY_SHININESS_STRENGTH, mat->attrib.sharpness);
			model->mMaterials[i]->Get(AI_MATKEY_COLOR_TRANSPARENT, mat->attrib.transFilter);
			model->mMaterials[i]->Get(AI_MATKEY_REFRACTI, mat->attrib.optDens);
			model->mMaterials[i]->Get(AI_MATKEY_COLOR_EMISSIVE, mat->attrib.emmReflect);
			mat->attrib.illum = 0;
			model->mMaterials[i]->Get(AI_MATKEY_NAME, mat->name);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), mat->mapKd);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_SPECULAR(0), mat->mapKs);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_AMBIENT(0), mat->mapKa);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_EMISSIVE(0), mat->mapKe);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_SHININESS(0), mat->mapNs);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_OPACITY(0), mat->mapD);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_DISPLACEMENT(0), mat->disp);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_LIGHTMAP(0), mat->decal);
			model->mMaterials[i]->Get(AI_MATKEY_TEXTURE_HEIGHT(0), mat->bump);

			materials.push_back(*mat);
		}	
	}

	for (int i = 0; i < model->mNumMeshes; i++)
	{
		const aiMesh* currMesh = model->mMeshes[i];

		for (int j = 0; j < currMesh->mNumVertices; j++)
		{
			GW::MATH2D::GVECTOR3F pos = (GW::MATH2D::GVECTOR3F&)currMesh->mVertices[j];
			GW::MATH2D::GVECTOR3F norm = { 0.0f, 1.0f, 0.0f };
			GW::MATH2D::GVECTOR2F uv = { 0.0f, 0.0f };

			if (currMesh->HasNormals())
				norm = (GW::MATH2D::GVECTOR3F&)currMesh->mNormals[j];

			if (currMesh->HasTextureCoords(0))
				uv = (GW::MATH2D::GVECTOR2F&)currMesh->mTextureCoords[0][j];

			positions.push_back(pos);
			uvs.push_back(uv);
			norms.push_back(norm);

			JointVertex vert = { pos, uv, norm };
			vertices.push_back(vert);
		}

		for (int j = 0; j < currMesh->mNumFaces; j++)
		{
			const aiFace& face = currMesh->mFaces[j];
			for (int k = 0; k < 3; k++)
			{
				indices.push_back(face.mIndices[k]);
			}
		}

		for (int j = 0; j < currMesh->mNumBones; j++)
		{
			const aiBone* currBone = currMesh->mBones[j];
			int boneId = GetBoneIndex(currBone);

			if (boneId == boneProps.size())
			{
				BoneProperties props;
				props.boneName = currBone->mName.C_Str();
				props.parentName = currBone->mNode->mParent->mName.C_Str();
				props.offsetMatrix = (GW::MATH::GMATRIXF&)(currBone->mOffsetMatrix);
				boneProps.push_back(props);
			}

			for (int k = 0; k < currBone->mNumWeights; k++)
			{
				const aiVertexWeight& vertWeight = currBone->mWeights[k];
				unsigned vertId = meshes[i].vertexStart + vertWeight.mVertexId;
				bones[vertId].AddBone(boneId, vertWeight.mWeight);
			}
		}

	}

	for (int i = 0; i < bones.size(); i++)
	{
		for (int j = 0; j < JOINTS_PER_VERTEX; j++)
		{
			vertices[i].joints.data[j] = (float)bones[i].id[j];
			vertices[i].weights.data[j] = bones[i].weight[j];
		}
	}

	for (int i = 0; i < boneProps.size(); i++)
	{
		boneProps[i].parentNdx = GetBoneIndex(boneProps[i].parentName);
	}

}

int MAD::Model::GetBoneIndex(const aiBone* bone)
{
	int boneNdx = 0;
	std::string boneName = bone->mName.C_Str();

	if (boneMap.find(boneName) == boneMap.end())
	{
		boneNdx = (int)boneMap.size();
		boneMap[boneName] = boneNdx;
	}
	else
	{
		boneNdx = boneMap[boneName];
	}
	return boneNdx;
}

int MAD::Model::GetBoneIndex(const std::string& bone)
{
	int boneNdx = -1;
	std::string boneName = bone;
	if (boneMap.find(boneName) != boneMap.end())
	{
		boneNdx = boneMap[boneName];
	}
	return boneNdx;
}

const aiNode* MAD::Model::FindSkeletonNode(const aiScene* model)
{
	const aiNode* currNode = model->mRootNode;
	std::queue<const aiNode*> queue;

	if (currNode)
	{
		queue.push(currNode);
	}

	while (!queue.empty())
	{
		currNode = queue.front();
		std::string temp = std::string(currNode->mName.C_Str());
		if (temp.find("Armature") != std::string::npos)
			return currNode;

		queue.pop();

		for (int i = 0; i < currNode->mNumChildren; i++)
		{
			queue.push(currNode->mChildren[i]);
		}
	}

	return nullptr;
}

std::vector<GW::MATH::GMATRIXF> MAD::Model::BoneTransform(float seconds, UINT animationNdx)
{
	if (!model->HasAnimations())
		return std::vector<GW::MATH::GMATRIXF>();


	if (animationNdx >= model->mNumAnimations) 
		return std::vector<GW::MATH::GMATRIXF>();

	const aiAnimation* animation = model->mAnimations[animationNdx];
	float animationTime = CalcAnimationTime(seconds, animation);

	GW::MATH::GMATRIXF identity = GW::MATH::GIdentityMatrixF;
	const aiNode* start = (skeleton) ? skeleton : model->mRootNode;
	ReadNodeHierarchy(animationTime, start, animation, identity);

	std::vector<GW::MATH::GMATRIXF> transforms;
	transforms.resize(boneProps.size());

	for (size_t i = 0; i < boneProps.size(); i++)
	{
		transforms[i] = boneProps[i].finalTransform;
	}

	return transforms;
}

std::vector<GW::MATH::GMATRIXF> MAD::Model::BoneTransformBlended(float seconds, UINT startAnimationNdx, UINT endAnimationNdx, float transitionTime)
{
	if (!model->HasAnimations())
		return std::vector<GW::MATH::GMATRIXF>();

	if (startAnimationNdx >= model->mNumAnimations || endAnimationNdx >= model->mNumAnimations)
		return std::vector<GW::MATH::GMATRIXF>();

	const aiAnimation* startAnimation = model->mAnimations[startAnimationNdx];
	float startAnimationTime = CalcAnimationTime(seconds, startAnimation);

	const aiAnimation* endAnimation = model->mAnimations[endAnimationNdx];
	float endAnimationTime = CalcAnimationTime(seconds, endAnimation);

	GW::MATH::GMATRIXF identity = GW::MATH::GIdentityMatrixF;

	const aiNode* start = (skeleton) ? skeleton : model->mRootNode;
	ReadNodeHierarchyBlended(transitionTime, startAnimationTime, endAnimationTime, start, startAnimation, endAnimation, identity);

	std::vector<GW::MATH::GMATRIXF> transforms;
	transforms.resize(boneProps.size());

	for (size_t i = 0; i < boneProps.size(); i++)
	{
		transforms[i] = boneProps[i].finalTransform;
	}

	return transforms;
}

void MAD::Model::ReadNodeHierarchy(float duration, const aiNode* node, const aiAnimation* animation, const GW::MATH::GMATRIXF& parentTransform)
{
	if (!node || !animation)
	{
		return;
	}

	std::string nodeName = node->mName.C_Str();

	GW::MATH::GMATRIXF nodeTransformation = (GW::MATH::GMATRIXF&)(node->mTransformation);

	const aiNodeAnim* nodeAnim = FindAnimationNode(animation, nodeName);

	LocalTransform localTransform = {};

	if (nodeAnim)
		CalcLocalTransform(localTransform, duration, nodeAnim);

	nodeTransformation = (GW::MATH::GMATRIXF&)(aiMatrix4x4(localTransform.scale, localTransform.rotation, localTransform.translation));

	GW::MATH::GMATRIXF global_matrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMatrix::MultiplyMatrixF(parentTransform, nodeTransformation, global_matrix);

	if (boneMap.find(nodeName) != boneMap.end())
	{		
		UINT bone_index = boneMap[nodeName];
		GW::MATH::GMATRIXF result_matrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMatrix::MultiplyMatrixF(globalInverse, global_matrix, result_matrix);
		GW::MATH::GMatrix::MultiplyMatrixF(result_matrix, boneProps[bone_index].offsetMatrix, boneProps[bone_index].finalTransform);

		boneProps[bone_index].skeletonMatrix = global_matrix;
	}

	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		ReadNodeHierarchy(duration, node->mChildren[i], animation, global_matrix);
	}
}

void MAD::Model::ReadNodeHierarchyBlended(float transitionTime, float startAnimDuration, float endAnimDuration, const aiNode* node, const aiAnimation* startAnimation, const aiAnimation* endAnimation, const GW::MATH::GMATRIXF& parentTransform)
{
	if (!node || !startAnimation || !endAnimation)
		return;

	std::string nodeName = node->mName.C_Str();

	GW::MATH::GMATRIXF nodeTransformation = (GW::MATH::GMATRIXF&)(node->mTransformation);

	const aiNodeAnim* startNodeAnim = FindAnimationNode(startAnimation, nodeName);

	const aiNodeAnim* endNodeAnim = FindAnimationNode(endAnimation, nodeName);

	//transitionTime = std::clamp(transitionTime, 0.0f, 1.0f);
	LocalTransform startTransform = {};
	if (startNodeAnim)
		CalcLocalTransform(startTransform, startAnimDuration, startNodeAnim);

	LocalTransform endTransform = {};
	if (endNodeAnim)
		CalcLocalTransform(endTransform, endAnimDuration, endNodeAnim);

	if (startNodeAnim && endNodeAnim)
	{
		aiVector3D blendedScale = startTransform.scale * (1.0f - transitionTime) + endTransform.scale * transitionTime;

		aiQuaternion blendedRotation = {};
		aiQuaternion::Interpolate(blendedRotation, startTransform.rotation, endTransform.rotation, transitionTime);
		blendedRotation.Normalize();
		
		aiVector3D blendedTranslation = startTransform.translation * (1.0f - transitionTime) + endTransform.translation * transitionTime;

		nodeTransformation = (GW::MATH::GMATRIXF&)(aiMatrix4x4(blendedScale, blendedRotation, blendedTranslation));
	}

	GW::MATH::GMATRIXF global_matrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMatrix::MultiplyMatrixF(parentTransform, nodeTransformation, global_matrix);

	if (boneMap.find(nodeName) != boneMap.end())
	{
		UINT bone_index = boneMap[nodeName];
		GW::MATH::GMATRIXF result_matrix = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMatrix::MultiplyMatrixF(globalInverse, global_matrix, result_matrix);
		GW::MATH::GMatrix::MultiplyMatrixF(result_matrix, boneProps[bone_index].offsetMatrix, boneProps[bone_index].finalTransform);

		boneProps[bone_index].skeletonMatrix = global_matrix;
	}

	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		ReadNodeHierarchyBlended(transitionTime, startAnimDuration, endAnimDuration, node->mChildren[i], startAnimation, endAnimation, global_matrix);
	}
}

const aiNodeAnim* MAD::Model::FindAnimationNode(const aiAnimation* animation, const std::string nodeName)
{
	if (animation)
	{
		for (size_t i = 0; i < animation->mNumChannels; i++)
		{
			const aiNodeAnim* nodeAnim = animation->mChannels[i];
			if (strcmp(nodeName.c_str(), nodeAnim->mNodeName.C_Str()) == 0)
			{
				return nodeAnim;
			}
		}
	}
	return nullptr;
}

void MAD::Model::CalcInterpolatedScalingVector(aiVector3D& out, float duration, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumScalingKeys == 1)
	{
		out = nodeAnim->mScalingKeys[0].mValue;
		return;
	}

	unsigned scaling_index = FindScalingAnimationIndex(duration, nodeAnim);
	unsigned next_scaling_index = (scaling_index + 1) % nodeAnim->mNumScalingKeys;
	if (next_scaling_index < nodeAnim->mNumScalingKeys)
	{
		float t1 = (float)nodeAnim->mScalingKeys[scaling_index].mTime;
		float t2 = (float)nodeAnim->mScalingKeys[next_scaling_index].mTime;
		float delta_time = t2 - t1;
		float factor = (duration - t1) / delta_time;
		if (factor >= 0.0f && factor <= 1.0f)
		{
			const aiVector3D& start = nodeAnim->mScalingKeys[scaling_index].mValue;
			const aiVector3D& end = nodeAnim->mScalingKeys[next_scaling_index].mValue;
			aiVector3D delta = end - start;
			out = start + factor * delta;
		}
	}
}

void MAD::Model::CalcInterpolatedRotationQuaternion(aiQuaternion& out, float duration, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumRotationKeys == 1)
	{
		out = nodeAnim->mRotationKeys[0].mValue;
		return;
	}

	unsigned rotation_index = FindRotationAnimationIndex(duration, nodeAnim);
	unsigned next_rotation_index = (rotation_index + 1) % nodeAnim->mNumRotationKeys;
	if (next_rotation_index < nodeAnim->mNumRotationKeys)
	{
		float t1 = (float)nodeAnim->mRotationKeys[rotation_index].mTime;
		float t2 = (float)nodeAnim->mRotationKeys[next_rotation_index].mTime;
		float delta_time = t2 - t1;
		float factor = (duration - t1) / delta_time;
		if (factor >= 0.0f && factor <= 1.0f)
		{
			const aiQuaternion& start = nodeAnim->mRotationKeys[rotation_index].mValue;
			const aiQuaternion& end = nodeAnim->mRotationKeys[next_rotation_index].mValue;
			aiQuaternion::Interpolate(out, start, end, factor);
			out.Normalize();
		}
	}
}

void MAD::Model::CalcInterpolatedTranslationVector(aiVector3D& out, float duration, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumPositionKeys == 1)
	{
		out = nodeAnim->mPositionKeys[0].mValue;
		return;
	}

	unsigned translation_index = FindTranslationAnimationIndex(duration, nodeAnim);
	unsigned next_translation_index = (translation_index + 1) % nodeAnim->mNumPositionKeys;
	if (next_translation_index < nodeAnim->mNumPositionKeys)
	{
		float t1 = (float)nodeAnim->mPositionKeys[translation_index].mTime;
		float t2 = (float)nodeAnim->mPositionKeys[next_translation_index].mTime;
		float delta_time = t2 - t1;
		float factor = (duration - t1) / delta_time;
		if (factor >= 0.0f && factor <= 1.0f)
		{
			const aiVector3D& start = nodeAnim->mPositionKeys[translation_index].mValue;
			const aiVector3D& end = nodeAnim->mPositionKeys[next_translation_index].mValue;
			aiVector3D delta = end - start;
			out = start + factor * delta;
		}
	}
}

void MAD::Model::CalcLocalTransform(LocalTransform& out, float duration, const aiNodeAnim* nodeAnim)
{
	CalcInterpolatedScalingVector(out.scale, duration, nodeAnim);
	CalcInterpolatedRotationQuaternion(out.rotation, duration, nodeAnim);
	CalcInterpolatedTranslationVector(out.translation, duration, nodeAnim);
}

float MAD::Model::CalcAnimationTime(float seconds, const aiAnimation* animation)
{
	float ticksPerSecond = (animation->mTicksPerSecond != 0) ? animation->mTicksPerSecond : 30.0f;
	float timeInTicks = seconds * ticksPerSecond;
	return (float)fmod(timeInTicks, animation->mDuration);
}

unsigned MAD::Model::FindScalingAnimationIndex(float duration, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumScalingKeys > 0)
	{
		for (unsigned i = 0; i < nodeAnim->mNumScalingKeys - 1; i++)
		{
			float t = nodeAnim->mScalingKeys[i + 1].mTime;
			if (duration < t) return i;
		}
	}
	return 0;
}

unsigned MAD::Model::FindRotationAnimationIndex(float duration, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumRotationKeys > 0)
	{
		for (unsigned i = 0; i < nodeAnim->mNumRotationKeys - 1; i++)
		{
			float t = nodeAnim->mRotationKeys[i + 1].mTime;
			if (duration < t) return i;
		}
	}
	return 0;
}

unsigned MAD::Model::FindTranslationAnimationIndex(float duration, const aiNodeAnim* nodeAnim)
{
	if (nodeAnim->mNumPositionKeys > 0)
	{
		for (unsigned i = 0; i < nodeAnim->mNumPositionKeys - 1; i++)
		{
			float t = (float)nodeAnim->mPositionKeys[i + 1].mTime;
			if (duration < t) return i;
		}
	}
	return 0;
}
