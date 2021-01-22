#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/StateGameObject.h"
#include "../CSC8503Common/StateObstacleObject.h"
#include "../CSC8503Common/PushdownMachine.h"
#include "../CSC8503Common/PushdownState.h"

namespace NCL {
	namespace CSC8503 {
		class CourseworkGame {
		public:
			CourseworkGame();
			~CourseworkGame();

			void DrawMainMenu();

			void DrawGameOver(std::string winner);

			bool UpdatePushdown(float dt) {
				if (!machine->Update(dt)) {
					return false;
				}
				return true;
			}

			virtual void UpdateGame(float dt);

		protected:
			PushdownMachine* machine;

			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			void InitGameExamples();

			void InitPlayer();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitDefaultFloor();
			void BridgeConstraintTest();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			vector<GameObject*> AddWallsToWorld(const Vector3& position);

			vector<GameObject*> AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);

			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			void InitEnemies();

			void InitCoins();

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			GameTechRenderer* renderer;
			PhysicsSystem* physics;
			GameWorld* world;

			GameObject* player = nullptr;
			std::string winnerName;

			vector<StateGameObject*> enemies;
			StateGameObject* AddStateEnemyToWorld(const Vector3& position);
			bool multi;

			vector<StateObstacleObject*> obstacles;

			StateObstacleObject* AddStateObstacleToWorld(const Vector3& position);

			GameObject* selectionObject = nullptr;

			OGLMesh* capsuleMesh = nullptr;
			OGLMesh* cubeMesh = nullptr;
			OGLMesh* sphereMesh = nullptr;
			OGLTexture* basicTex = nullptr;
			OGLShader* basicShader = nullptr;

			//Coursework Meshes
			OGLMesh* charMeshA = nullptr;
			OGLMesh* charMeshB = nullptr;
			OGLMesh* enemyMesh = nullptr;
			OGLMesh* bonusMesh = nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject = nullptr;
			float lockedPitch = 0;
			void LockCameraToPlayer(GameObject* p) {
				lockedObject = p;
			}

			float elapsedTime = 0;

			bool initialised = false;

			bool WinConditionMet(GameObject* testSubject);

			class PauseScreen : public PushdownState {
			public:
				PauseScreen(CourseworkGame* cw) {
					this->cw = cw;
				}

				PushdownResult OnUpdate(float dt, PushdownState** newState) override {
					
					if (pauseRemain < 0) {
						if (Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
							return PushdownResult::Pop;
						}
					}
					else {
						pauseRemain -= dt;
					}
					

					cw->UpdateGame(0);

					return PushdownResult::NoChange;
				}
				void OnAwake() override {
					std::cout << "Press ESC to unpause the game!\n";
					pauseRemain = 0.2;
				}
			protected:
				CourseworkGame* cw;
				float pauseRemain = 1;
			};

			class GameScreen : public PushdownState {
			public:

				GameScreen(CourseworkGame* cw, bool multi = 0) {
					cw->multi = multi;
					this->cw = cw;
				}

				PushdownResult OnUpdate(float dt, PushdownState** newState) override {
					
					if (!cw->winnerName.empty() || cw->player->GetScore() == 0) {
						std::cout << "win\n";
						*newState = new GameOverScreen(cw, cw->winnerName);
						return PushdownResult::Push;
					}

					if (pauseRemain < 0) {
						if (Window::GetKeyboard()->KeyDown(KeyboardKeys::F1)) {
							return PushdownResult::Pop;
						}
						if (Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
							*newState = new PauseScreen(cw);
							return PushdownResult::Push;
						}
					}
					else {
						pauseRemain -= dt;
					}

					cw->UpdateGame(dt);

					return PushdownResult::NoChange;
				}

				void OnAwake() override {
					std::cout << "Resuming Game\n";
					pauseRemain = 0.2;
				}

			protected:
				CourseworkGame* cw;

				bool multiplayer;
				float pauseRemain = 1;
			};

			class IntroScreen : public PushdownState {
			public:
				IntroScreen(CourseworkGame* cw) {
					this->cw = cw;
				}

				PushdownResult OnUpdate(float dt, PushdownState** newState) override {
					if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
						*newState = new GameScreen(cw, 0);
						return PushdownResult::Push;
					}
					if (Window::GetKeyboard()->KeyDown(KeyboardKeys::M)) {
						*newState = new GameScreen(cw, 1);
						return PushdownResult::Push;
					}
					if (Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
						return PushdownResult::Pop;
					}

					cw->DrawMainMenu();

					return PushdownState::NoChange;
				}

				void OnAwake() override {
					std::cout << "Press P to play single player or M for multiplayer. ESC to quit\n";
				}

				void OnSleep() override {
					cw->InitialiseAssets();
				}

			protected:
				CourseworkGame* cw;
			};

			class GameOverScreen : public PushdownState {
			public:
				GameOverScreen(CourseworkGame* cw, std::string winner) {
					this->cw = cw;
					this->winner = winner;
				}

				PushdownResult OnUpdate(float dt, PushdownState** newState) override {
					if (Window::GetKeyboard()->KeyDown(KeyboardKeys::F1)) {
						cw->machine->Set(new IntroScreen(cw));
					}

					cw->DrawGameOver(winner);

					return PushdownState::NoChange;
				}

				void OnAwake() override {
					std::cout << "Press P to play single player or M for multiplayer. ESC to quit\n";
				}

				void OnSleep() override {
					cw->InitialiseAssets();
				}

			protected:
				CourseworkGame* cw;
				std::string winner;
			};
			

		};

		/*void TestPushdownAutomata(Window* w) {
			PushdownMachine machine(new IntroScreen());
			while (w->UpdateWindow()) {
				float dt = w->GetTimer()->GetTimeDeltaSeconds();
				if (!machine.Update(dt)) {
					return;
				}
			}
		}*/
	}
}

