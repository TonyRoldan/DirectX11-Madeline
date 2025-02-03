#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <DirectXMath.h>

#include <string>
#include <filesystem>
#include "../Precompiled.h"
#include "../GameConfig.h"

#define JOINTS_PER_VERTEX 4

#define ASSIMP_FLAGS (					\
	aiProcess_ConvertToLeftHanded |			\
	aiProcessPreset_TargetRealtime_MaxQuality |	\
	aiProcess_LimitBoneWeights |			\
	aiProcess_ImproveCacheLocality |		\
	aiProcess_GlobalScale |					\
	aiProcess_PopulateArmatureData)

namespace MAD
{
	struct Attributes
	{
		aiColor3D diffReflect; float dissolve;
		aiColor3D specReflect; float spec;
		aiColor3D ambReflect; float sharpness;
		aiColor3D transFilter; float optDens;
		aiColor3D emmReflect; unsigned illum;
	};

	struct Material
	{
		Attributes attrib;
		const char* name;
		const char* mapKd;
		const char* mapKs;
		const char* mapKa;
		const char* mapKe;
		const char* mapNs;
		const char* mapD;
		const char* disp;
		const char* decal;
		const char* bump;
		const void* padding[2];
	};

	struct Mesh
	{
		unsigned indexCount;
		unsigned vertexStart;
		unsigned indexStart;
		unsigned materialStart;
	};

	struct LocalTransform
	{
		aiVector3D scale;
		aiQuaternion rotation;
		aiVector3D translation;
	};

	struct JointVertex
	{
		GW::MATH2D::GVECTOR3F pos;
		GW::MATH2D::GVECTOR2F uv;
		GW::MATH2D::GVECTOR3F norm;
		GW::MATH::GVECTORF weights;
		GW::MATH::GVECTORF joints;
	};

	struct VertexBoneData
	{
	private:
		unsigned ndx = 0;

	public:
		float weight[JOINTS_PER_VERTEX] = { 0.0f };
		unsigned id[JOINTS_PER_VERTEX] = { 0 };

		void AddBone(unsigned _id, float _weight)
		{
			for (int i = 0; i < ndx; i++)
			{
				if (id[i] == _id)
					return;
			}
			if (_weight == 0.0f)
				return;	
			if (ndx == JOINTS_PER_VERTEX)
				return;			

			weight[ndx] = _weight;
			id[ndx] = _id;

			ndx++;		
		}
	};

	struct BoneProperties
	{
		std::string boneName;
		std::string parentName;
		unsigned parentNdx;

		GW::MATH::GMATRIXF offsetMatrix;
		GW::MATH::GMATRIXF finalTransform;
		GW::MATH::GMATRIXF skeletonMatrix;
	};

	class Model
	{
	private:
		Assimp::Importer* parser;	
		const aiNode* skeleton;

		std::vector<GW::MATH2D::GVECTOR3F> positions;
		std::vector<GW::MATH2D::GVECTOR2F> uvs;
		std::vector<GW::MATH2D::GVECTOR3F> norms;

		std::vector<VertexBoneData> bones;
		std::map<std::string, unsigned> boneMap;
		std::vector<BoneProperties> boneProps;

		GW::MATH::GMATRIXF globalInverse;

		void ParseModel(const aiScene* model);
		int GetBoneIndex(const aiBone* bone);
		int GetBoneIndex(const std::string& bone);
		const aiNode* FindSkeletonNode(const aiScene* scene);
		std::vector<GW::MATH::GMATRIXF> BoneTransform(float seconds, UINT animationNdx);
		std::vector<GW::MATH::GMATRIXF> BoneTransformBlended(float seconds, UINT startAnimationNdx, UINT endAnimationNdx, float transitionTime);
		void ReadNodeHierarchy(float duration, const aiNode* node, const aiAnimation* animation, const GW::MATH::GMATRIXF& parentTransform);
		void ReadNodeHierarchyBlended(float transitionTime, float startAnimDuration, float endAnimDuration, const aiNode* node, const aiAnimation* startAnimation, const aiAnimation* endAnimation, const GW::MATH::GMATRIXF& parentTransform);
		const aiNodeAnim* FindAnimationNode(const aiAnimation* animation, const std::string nodeName);
		void CalcInterpolatedScalingVector(aiVector3D& out, float duration, const aiNodeAnim* nodeAnim);
		void CalcInterpolatedRotationQuaternion(aiQuaternion& out, float duration, const aiNodeAnim* nodeAnim);
		void CalcInterpolatedTranslationVector(aiVector3D& out, float duration, const aiNodeAnim* nodeAnim);
		void CalcLocalTransform(LocalTransform& out, float duration, const aiNodeAnim* nodeAnim);
		float CalcAnimationTime(float seconds, const aiAnimation* animation);
		unsigned FindScalingAnimationIndex(float duration, const aiNodeAnim* nodeAnim);
		unsigned FindRotationAnimationIndex(float duration, const aiNodeAnim* nodeAnim);
		unsigned FindTranslationAnimationIndex(float duration, const aiNodeAnim* nodeAnim);

	public:
		const aiScene* model;
		std::string modelName;

		std::vector<Mesh> meshes;
		std::vector<JointVertex> vertices;
		std::vector<unsigned> indices;
		std::vector<Material> materials;

		unsigned vertexStart;
		unsigned indexStart;
		unsigned materialStart;

		std::vector<GW::MATH::GMATRIXF> currPose;
		std::vector<JointVertex> skeletonVerts;
		GW::MATH::GMATRIXF world;

		Model();
		Model(const Model& other);
		Model& operator =(const Model& other);
		~Model();
		bool LoadModel(const std::string& fileName, const std::string& fbxName);
		void UpdatePose(float duration, unsigned animationNdx);
		void UpdatePoseBlended(float duration, unsigned startAnimationNdx, unsigned endEnimationNdx, float transitionTime);
		
	};

	inline aiVector3D operator*(const aiVector3D& lhs, const aiVector3D& rhs)
	{
		return aiVector3D(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
	}

	inline aiVector3D operator*(const aiVector3D& vec, float scalar)
	{
		return aiVector3D(vec.x * scalar, vec.y * scalar, vec.z * scalar);
	}

	inline aiVector3D operator*(float scalar, const aiVector3D& vec)
	{
		return vec * scalar;
	}

	inline aiQuaternion operator*(const aiQuaternion& q1, const aiQuaternion& q2)
	{
		return aiQuaternion(
			q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
			q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
			q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
			q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w
		);
	}

	inline aiQuaternion operator*(const aiQuaternion& quat, float scalar)
	{
		return aiQuaternion(quat.w * scalar, quat.x * scalar, quat.y * scalar, quat.z * scalar);
	}
};

