#pragma once
#include "..\CSC8503Common\GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class StateMachine;

		class StateObstacleObject : public GameObject
		{
		public:
			StateObstacleObject(std::string name);
			~StateObstacleObject();

			virtual void Update(float dt);

		protected:
			void MoveUp(float dt);
			void MoveDown(float dt);

			StateMachine* stateMachine;

			float counter;
		};
	}
}

