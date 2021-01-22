#pragma once
#include "..\CSC8503Common\GameObject.h"
#include "..\CSC8503Common\NavigationGrid.h"
#include "..\CSC8503Common\NavigationPath.h"

namespace NCL {
	namespace CSC8503 {
		class StateMachine;

		class StateGameObject : public GameObject
		{
		public:
			StateGameObject(std::string name);
			~StateGameObject();

			virtual void Update(float dt);

			bool TestForPath();

		protected:
			void MoveLeft(float dt);
			void MoveRight(float dt);

			void Pathfind(float dt);

			void Jump(float dt);

			StateMachine* stateMachine;

			float counter;
			bool pathFound;

			NavigationGrid* grid;
			vector<Vector3> pathNodes;
		};
	}
}
