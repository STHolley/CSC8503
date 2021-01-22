#pragma once
#include "Transform.h"
#include "CollisionVolume.h"

#include "PhysicsObject.h"
#include "RenderObject.h"

#include <vector>

using std::vector;

namespace NCL {
	namespace CSC8503 {

		class GameObject {
		public:
			GameObject(string name = "");
			~GameObject();

			void SetBoundingVolume(CollisionVolume* vol) {
				boundingVolume = vol;
			}

			const CollisionVolume* GetBoundingVolume() const {
				return boundingVolume;
			}

			bool IsActive() const {
				return isActive;
			}

			void Activate() {
				isActive = true;
			}

			void Deactivate() {
				isActive = false;
			}

			Transform& GetTransform() {
				return transform;
			}

			RenderObject* GetRenderObject() const {
				return renderObject;
			}

			PhysicsObject* GetPhysicsObject() const {
				return physicsObject;
			}

			void SetRenderObject(RenderObject* newObject) {
				renderObject = newObject;
			}

			void SetPhysicsObject(PhysicsObject* newObject) {
				physicsObject = newObject;
			}

			const string& GetName() const {
				return name;
			}

			virtual void OnCollisionBegin(GameObject* otherObject) {
				//std::cout << "OnCollisionBegin event occured!\n";
				//Is currently in collision with a 0 invmass (fixed) object and therefore can jump
				if (otherObject->GetTransform().GetPosition().y < GetTransform().GetPosition().y && otherObject->GetName() == "World") {
					grounded = true;
				}
				if ((name == "Player" || name == "Enemy") && otherObject->GetName() == "Coin") {
					IncrementScore();
					otherObject->Deactivate();
					otherObject->SetBoundingVolume(nullptr);
				}
				if ((otherObject->GetName() == "Player" || otherObject->GetName() == "Enemy") && name == "Coin") {
					otherObject->IncrementScore();
					Deactivate();
					SetBoundingVolume(nullptr);
				}
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {
				grounded = false;
			}

			bool IsGrounded() {
				return grounded;
			}

			bool GetBroadphaseAABB(Vector3&outsize) const;

			void UpdateBroadphaseAABB();

			void SetWorldID(int newID) {
				worldID = newID;
			}

			int		GetWorldID() const {
				return worldID;
			}

			int		GetScore() const {
				return score;
			}

			void	SetScore(int s) {
				score = abs(s);
			}

			void	DecrementScore() {
				score = score - 10 < 0 ? 0 : score - 10;
			}

			void	IncrementScore() {
				score = score + 25;
			}

		protected:
			Transform			transform;

			bool grounded = false;

			CollisionVolume*	boundingVolume;
			PhysicsObject*		physicsObject;
			RenderObject*		renderObject;

			bool	isActive;
			int		score = 1000;
			int		worldID;
			string	name;

			Vector3 broadphaseAABB;
		};
	}
}

