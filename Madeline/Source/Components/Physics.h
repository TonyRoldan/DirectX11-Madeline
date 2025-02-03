// define all ECS components related to movement & collision
#ifndef PHYSICS_H
#define PHYSICS_H

// example space game (avoid name collisions)
using namespace GW::MATH;

namespace MAD 
{
	struct Offset { float value; };
	struct Velocity { GVECTORF value; };
	struct Acceleration { GVECTORF value; };

	struct RenderCollider {};
	struct Collidable {};
	struct PhysicsCollidable {};
	struct Triggerable {};
	struct Moveable {};


#pragma region Transform
	struct Transform 
	{ 
		GMATRIXF value; 
		
		Transform() : value(GIdentityMatrixF) {}

		Transform(GMATRIXF _value) : value(_value) {}

		Transform(std::string _iniName, std::weak_ptr<const GameConfig> _gameConfig)
		{
			std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
			if (readCfg->find(_iniName.c_str()) == readCfg->end())
			{
				value = GZeroMatrixF;
				return;
			}

			value = GIdentityMatrixF;

			bool hasScale = false, hasRotation = false, hasPosition = false;

			if (readCfg->at(_iniName.c_str()).find("scale") != readCfg->at(_iniName.c_str()).end())
			{
				GVECTORF scale = StringToGVector(readCfg->at(_iniName.c_str()).at("scale").as<std::string>());
				scale = MultiplyVector(scale, .5f);
				GMatrix::ScaleLocalF(value, scale, value);
				hasScale = true;
			}
			if (readCfg->at(_iniName.c_str()).find("rotation") != readCfg->at(_iniName.c_str()).end())
			{
				GVECTORF dir = StringToGVector(readCfg->at(_iniName.c_str()).at("rotation").as<std::string>());
				GMatrix::RotateXLocalF(value, G_DEGREE_TO_RADIAN_F(dir.x), value);
				GMatrix::RotateYLocalF(value, G_DEGREE_TO_RADIAN_F(dir.y), value);
				GMatrix::RotateZLocalF(value, G_DEGREE_TO_RADIAN_F(dir.z), value);
				hasRotation = true;
			}
			if (readCfg->at(_iniName.c_str()).find("position") != readCfg->at(_iniName.c_str()).end())
			{
				GVECTORF pos = StringToGVector(readCfg->at(_iniName.c_str()).at("position").as<std::string>());
				value.row4 = { pos.x, pos.y, pos.z, 1 };
				hasPosition = true;
			}

			if (!hasScale && !hasRotation && !hasPosition)
				value = GZeroMatrixF;
		}
	};
#pragma endregion

#pragma region Rays
	struct RaycastHit
	{
		GVECTORF contactPoint;
		GVECTORF surfaceNormal;
		float contactTime;
	};

	struct Ray
	{
		GVECTORF origin;
		GVECTORF dir;

		bool RaycastBox2D(const GAABBMMF& _collider, RaycastHit& _hitResult)
		{

			GVECTORF nearTime;
			GVector::SubtractVectorF(_collider.min, origin, nearTime);
			nearTime = DivideVector(nearTime, dir);
			GVECTORF farTime;
			GVector::SubtractVectorF(_collider.max, origin, farTime);
			farTime = DivideVector(farTime, dir);

			if (std::isnan(farTime.x) || std::isnan(farTime.y)) return false;
			if (std::isnan(nearTime.x) || std::isnan(nearTime.y)) return false;

			if (nearTime.x > farTime.x) std::swap(nearTime.x, farTime.x);
			if (nearTime.y > farTime.y) std::swap(nearTime.y, farTime.y);

			if (nearTime.x > farTime.y || nearTime.y > farTime.x) return false;

			_hitResult.contactTime = max(nearTime.x, nearTime.y);
			float tHitFar = (((farTime.x) < (farTime.y)) ? (farTime.x) : (farTime.y));


			if (tHitFar < 0) return false;

			GVector::AddVectorF(origin, MultiplyVector(dir, _hitResult.contactTime), _hitResult.contactPoint);

			if (nearTime.x > nearTime.y)
				if (dir.x < 0)
					_hitResult.surfaceNormal = { 1, 0, 0 , 0 };
				else
					_hitResult.surfaceNormal = { -1, 0, 0, 0 };
			else
				if (dir.y < 0)
					_hitResult.surfaceNormal = { 0, 1, 0, 0 };
				else
					_hitResult.surfaceNormal = { 0, -1, 0, 0  };

			return true;
		}
	};
#pragma endregion

#pragma region Colliders
	enum ColliderType { BOX };

	struct Collider
	{
		flecs::id ownerId;
		ColliderType type;
		size_t id;
		bool isTrigger;
		bool isOneWay;
		GVECTORF localPos;
		std::vector<Collider*> contacts;

		Collider(ColliderType _type, size_t _id, bool _isTrigger, bool _isOneWay) : 
			ownerId(flecs::id()), 
			type(_type), 
			id(_id), 
			isTrigger(_isTrigger),
			isOneWay(_isOneWay),
			localPos({}), 
			contacts({})
		{}

	public:
		bool IsContacting(Collider* _collider) const
		{
			for (Collider* contact : contacts)
			{
				if (contact == _collider)
					return true;
			}

			return false;
		}

		void EnterContact(Collider* _contact)
		{
			contacts.push_back(_contact);
		}

		void ExitContact(Collider* _contact)
		{
			for (auto contact = contacts.begin(); contact != contacts.end(); contact++)
			{
				if (*contact == _contact)
				{
					contacts.erase(contact);
					return;
				}
			}
		}

		void DropAllContacts()
		{
			contacts.clear();
		}

		void DropCollidersContacts(std::vector<std::shared_ptr<Collider>>& _colliders)
		{
			std::vector<Collider*> contactsToDrop;
			for (auto collider : _colliders)
			{
				for (auto contact = contacts.begin(); contact != contacts.end(); contact++)
				{
					if (collider.get() == *contact)
					{
						contacts.erase(contact);
						break;
					}
				}
			}
		}

		virtual GCollision::GCollisionCheck CollisionCheck(const Collider* _other) const = 0;

		virtual GCollision::GCollisionCheck DynamicCollisionCheck2D(
			const Collider* _other, 
			const GVECTORF& _amountToMove,
			RaycastHit& _hitResult) const = 0;

		virtual GVECTORF GetClosestPoint(const Collider* _other) const = 0;

		virtual inline GVECTORF GetCenter() const = 0;

		virtual void UpdateWorldPosition(const GVECTORF& _worldPos) = 0;

		virtual GVECTORF GetExtent() const = 0;
	};

	struct BoxCollider : Collider
	{
		GAABBMMF boundBox;
		GVECTORF size;

		BoxCollider(size_t _id, bool _isTrigger, bool _isOneWay, GAABBMMF _boundBox)
			: boundBox(_boundBox), Collider(BOX, _id, _isTrigger, _isOneWay)
		{
			size = {
				boundBox.max.x - boundBox.min.x,
				boundBox.max.y - boundBox.min.y,
				boundBox.max.z - boundBox.min.z,
				1
			};
		}

		GCollision::GCollisionCheck CollisionCheck(const Collider* _other) const override
		{
			GCollision::GCollisionCheck collisionCheck = GCollision::GCollisionCheck::NO_COLLISION;

			switch (_other->type)
			{
			case BOX:
			{
				GAABBCEF curBox;
				GAABBCEF otherBox;
				GCollision::ConvertAABBMMToAABBCEF(boundBox, curBox);
				GCollision::ConvertAABBMMToAABBCEF(((BoxCollider*)_other)->boundBox, otherBox);
				GCollision::TestAABBToAABBF(curBox, otherBox, collisionCheck);
				break;
			}
			default:
				break;
			}

			return collisionCheck;
		}

		GCollision::GCollisionCheck DynamicCollisionCheck2D(
			const Collider* _other,
			const GVECTORF& _amountToMove,
			RaycastHit& _hitResult) const override
		{
			GCollision::GCollisionCheck collisionCheck = GCollision::GCollisionCheck::NO_COLLISION;

			if (_amountToMove.x == 0 && _amountToMove.y == 0)
				return collisionCheck;

			switch (_other->type)
			{
			case BOX:
			{
				if (_other->isOneWay)
				{
					if (_amountToMove.y >= 0)
						return collisionCheck;
					else if (CollisionCheck(_other) != GCollision::GCollisionCheck::NO_COLLISION)
						return collisionCheck;
				}

				BoxCollider* box = (BoxCollider*)_other;
				// The collider is expanded by half the width of our collider, so that our contact point
				// is exactly where we need to end up to not enter the _other collider
				GAABBMMF expandedBox = box->boundBox; 
				GVector::SubtractVectorF(expandedBox.min, MultiplyVector(size, .5f), expandedBox.min);
				GVector::AddVectorF(expandedBox.max, MultiplyVector(size, .5f), expandedBox.max);
				Ray ray
				{
					GetGAABBMMFCenter(boundBox),
					_amountToMove
				};

				if (ray.RaycastBox2D(expandedBox, _hitResult))
				{
					// Time is how far along the ray it hit the box, 
					// if it's greater than 1, with current velocity this will not hit the other collider.
					if (_hitResult.contactTime <= 1.0f)
						return GCollision::GCollisionCheck::COLLISION;
				}
				break;
			}
			default:
				break;
			}

			return collisionCheck;
		}
		
		GVECTORF GetClosestPoint(const Collider* _other) const override
		{
			GVECTORF closestPoint;
			GCollision::ClosestPointToAABBF(boundBox, _other->GetCenter(), closestPoint);
			return closestPoint;
		} 

		inline GVECTORF GetCenter() const override
		{
			return GetGAABBMMFCenter(boundBox);
		}

		void UpdateWorldPosition(const GVECTORF& _worldPos) override
		{
			GVECTORF center;
			GVector::AddVectorF(_worldPos, localPos, center);
			boundBox.min = {
				center.x - (size.x * .5f),
				center.y - (size.y * .5f),
				center.z - (size.z * .5f)
			};

			boundBox.max = {
				center.x + (size.x * .5f),
				center.y + (size.y * .5f),
				center.z + (size.z * .5f)
			};
		}

		GVECTORF GetExtent() const override
		{
			return GetGAABBMMFExtent(boundBox);
		}
	};

	struct ColliderContainer 
	{
		flecs::id ownerId;
		bool isMoveable;
		GW::MATH::GVECTORF worldPos;
		float maxColDist;
		std::vector<std::shared_ptr<Collider>> colliders;
		std::vector<std::shared_ptr<Collider>> triggerColliders;
		std::vector<std::shared_ptr<Collider>> physicsColliders;
		std::vector<ColliderContainer*> contacts;

#pragma region Constructors / Destructor
		ColliderContainer()
		{
			ownerId = flecs::id();
			isMoveable = false;
			worldPos = {};
			maxColDist = 0;
			colliders = {};
			triggerColliders = {};
			physicsColliders = {};
			contacts = {};
		}

		ColliderContainer(bool _isMoveable)
		{
			ownerId = flecs::id();
			isMoveable = _isMoveable;
			worldPos = {};
			maxColDist = 0;
			colliders = {};
			triggerColliders = {};
			physicsColliders = {};
			contacts = {};
		}

		ColliderContainer(bool _isMoveable, std::string _iniName, std::weak_ptr<const GameConfig> _gameConfig)
		{
			ownerId = flecs::id();
			isMoveable = _isMoveable;
			worldPos = {};
			maxColDist = 0;
			colliders = {};
			triggerColliders = {};
			physicsColliders = {};
			contacts = {};

			std::shared_ptr<const GameConfig> readCfg = _gameConfig.lock();
			if (readCfg->find(_iniName.c_str()) == readCfg->end())
				return;

			int i = 0;
			std::string iStr = std::to_string(i);
			// Add all box colliders
			while (readCfg->at(_iniName.c_str()).find("boxCol" + iStr + "IsTrigger") != readCfg->at(_iniName.c_str()).end())
			{
				bool isOneWay = false;
				bool isTrigger = readCfg->at(_iniName.c_str()).at("boxCol" + iStr + "IsTrigger").as<bool>();
				GVECTORF pos = GZeroVectorF;
				GVECTORF scale = { 1,1,1 };
				GVECTORF collisionDir = GZeroVectorF;

				if (readCfg->at(_iniName.c_str()).find("boxCol" + iStr + "IsOneWay") != readCfg->at(_iniName.c_str()).end())
					isOneWay = readCfg->at(_iniName.c_str()).at("boxCol" + iStr + "IsOneWay").as<bool>();

				if (readCfg->at(_iniName.c_str()).find("boxCol" + iStr + "Pos") != readCfg->at(_iniName.c_str()).end())
					pos = StringToGVector(readCfg->at(_iniName.c_str()).at("boxCol" + iStr + "Pos").as<std::string>());

				if (readCfg->at(_iniName.c_str()).find("boxCol" + iStr + "Scale") != readCfg->at(_iniName.c_str()).end())
					scale = StringToGVector(readCfg->at(_iniName.c_str()).at("boxCol" + iStr + "Scale").as<std::string>());

				if (readCfg->at(_iniName.c_str()).find("boxCol" + iStr + "ColDir") != readCfg->at(_iniName.c_str()).end())
					collisionDir = StringToGVector(readCfg->at(_iniName.c_str()).at("boxCol" + iStr + "ColDir").as<std::string>());

				AddBoxCollider(isTrigger, isOneWay, pos, scale, collisionDir);
				iStr = std::to_string(++i);
			}
		}

		ColliderContainer(const ColliderContainer& _other, flecs::id _ownerId, GVECTORF _worldPosition)
		{
			ownerId = _ownerId;
			isMoveable = _other.isMoveable;
			worldPos = _worldPosition;
			maxColDist = _other.maxColDist;
			colliders = {};
			triggerColliders = {};
			physicsColliders = {};
			contacts = {};

			for (auto collider : _other.colliders)
			{
				CopyCollider(collider.get());
			}

			UpdateWorldPosition(_worldPosition);
		}

		~ColliderContainer()
		{
			triggerColliders.clear();
			physicsColliders.clear();
			for (std::shared_ptr<Collider> collider : colliders)
			{
				collider.reset();
			}
		}
#pragma endregion

		inline bool InCollisionRange(const ColliderContainer& _other) const
		{
			return Distance2D(worldPos, _other.worldPos) < maxColDist + _other.maxColDist;
		}

		void UpdateWorldPosition(const GVECTORF& _worldPos)
		{
			worldPos = _worldPos;

			for (auto collider : colliders)
			{
				collider->UpdateWorldPosition(_worldPos);
			}
		}

		void AddBoxCollider(
			bool _isTrigger, 
			bool _isOneWay,
			const GVECTORF& _localPos, 
			const GVECTORF& _scale, 
			const GVECTORF& _collisionDir = GZeroVectorF)
		{
			GVECTORF halfScale = MultiplyVector(_scale, .5f);
			GVECTORF min;
			GVECTORF max;
			GVector::SubtractVectorF(_localPos, halfScale, min);
			GVector::AddVectorF(_localPos, halfScale, max);

			GAABBMMF boundBox
			{
				min,
				max
			};

			std::shared_ptr<BoxCollider> collider = std::make_shared<BoxCollider>(colliders.size(), _isTrigger, _isOneWay, boundBox);
			collider->localPos = _localPos;

			colliders.push_back(collider);

			if (_isTrigger)
				triggerColliders.push_back(colliders[colliders.size() - 1]);
			else
				physicsColliders.push_back(colliders[colliders.size() - 1]);

			UpdateMaxColDist();
		}

		void EnterContacts(ColliderContainer* _colliders)
		{
			if (std::find(contacts.begin(), contacts.end(), _colliders) == contacts.end())
				contacts.push_back(_colliders);
		}

		void ExitContacts(ColliderContainer* _colliders)
		{
			for (auto contact = contacts.begin(); contact != contacts.end(); contact ++)
			{
				if (_colliders == *contact)
				{

					contacts.erase(contact);

					break;
				}
			}

			for (auto collider : colliders)
			{
				collider->DropCollidersContacts(_colliders->colliders);
			}
		}

		void DropAllContacts()
		{
			for (auto contact : contacts)
			{
				contact->ExitContacts(this);
			}
			contacts.clear();

			for (auto collider : colliders)
			{
				collider->DropAllContacts();
			}
		}

		bool IsContacting(ColliderContainer* _contact) const
		{
			for (auto contact : contacts)
			{
				if (contact == _contact)
				{
					return true;
				}
			}

			return false;
		}

	private:
		void CopyCollider(Collider* collider)
		{
			std::shared_ptr<Collider> colliderCopy;

			switch (collider->type)
			{
			case BOX:
			{
				colliderCopy = std::make_shared<BoxCollider>(
					colliders.size(), 
					collider->isTrigger, 
					collider->isOneWay,
					((BoxCollider*)collider)->boundBox);
				break;
			}
			default:
				return;
			}

			colliderCopy->localPos = collider->localPos;
			colliderCopy->ownerId = ownerId;

			colliders.push_back(colliderCopy);
			if (collider->isTrigger)
				triggerColliders.push_back(colliderCopy);
			else
				physicsColliders.push_back(colliderCopy);
		}

		void UpdateMaxColDist()
		{
			maxColDist = 0;

			GVECTORF sumPos;
			float tempDist;
			for (auto collider : colliders)
			{
				GVector::AddVectorF(collider->localPos, collider->GetExtent(), sumPos);
				tempDist = Distance2D(GIdentityVectorF, sumPos);
				if (tempDist > maxColDist)
					maxColDist = tempDist;
			}
		}
	};
#pragma endregion
};

#endif