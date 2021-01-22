#include "CourseworkGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "../CSC8503Common/StateObstacleObject.h"

using namespace NCL;
using namespace CSC8503;

CourseworkGame::CourseworkGame() {
	machine = new PushdownMachine(new IntroScreen(this));
	world = new GameWorld();
	renderer = new GameTechRenderer(*world);
	physics = new PhysicsSystem(*world);

	Debug::SetRenderer(renderer);

	//InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes,
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void CourseworkGame::InitialiseAssets() {
	if (!initialised) {
		auto loadFunc = [](const string& name, OGLMesh** into) {
			*into = new OGLMesh(name);
			(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
			(*into)->UploadToGPU();
		};

		loadFunc("cube.msh", &cubeMesh);
		loadFunc("sphere.msh", &sphereMesh);
		loadFunc("Male1.msh", &charMeshA);
		loadFunc("courier.msh", &charMeshB);
		loadFunc("security.msh", &enemyMesh);
		loadFunc("coin.msh", &bonusMesh);
		loadFunc("capsule.msh", &capsuleMesh);

		basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
		basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");
		
		initialised = true;
	}
	InitCamera();
	InitWorld();
}

CourseworkGame::~CourseworkGame() {
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
	delete player;
	for (auto& i: enemies) {
		delete i;
	}
	for (auto& i : obstacles) {
		delete i;
	}
}

void CourseworkGame::DrawMainMenu() {
	glClearColor(0, 0, 0, 1);
	Debug::FlushRenderables(0);
	renderer->DrawString("Welcome to this Scuffed Game", Vector2(30, 10));
	renderer->DrawString("Press 'P' for single player practice", Vector2(30, 30));
	renderer->DrawString("Press 'M' for multiplayer with AI", Vector2(30, 50));
	renderer->DrawString("Press Esc to quit", Vector2(30, 70));
	renderer->Render();
	world->ClearAndErase();
	enemies.clear();
	obstacles.clear();
	player = nullptr;
	physics->Clear();
	winnerName.clear();
}

void CourseworkGame::DrawGameOver(std::string winner) {
	world->GetMainCamera()->SetPosition(Vector3(0, 10000, 0));
	renderer->DrawString("Game Over", Vector2(30, 40));
	if (winner.empty()) {
		renderer->DrawString("There is no winner :(", Vector2(30, 50));
	}
	else {
		renderer->DrawString("Winner: " + winner, Vector2(30, 50));
	}
	renderer->DrawString("Your Score: " + std::to_string(player->GetScore()), Vector2(30, 60));
	renderer->DrawString("Press F1 to return to the title screen", Vector2(20, 70));
	renderer->Render();
}

void CourseworkGame::UpdateGame(float dt) {
	glClearColor(1, 1, 1, 1);
	elapsedTime -= dt;

	float currPitch = world->GetMainCamera()->GetPitch();

	if (dt == 0) { //Only possible on pause or freaky PCs with instantaneous processing times. safe to assume a pause was made tbh
		//Paused state: Show pause text
		renderer->DrawString("Game Paused (ESC)", Vector2(30, 10));

	}
	else {
		world->GetMainCamera()->UpdateCamera(dt);
		lockedPitch += (currPitch - world->GetMainCamera()->GetPitch());


		UpdateKeys();
		if (multi) {
			for (StateGameObject* e : enemies) {
				e->Update(dt);
			}
		}
		for (StateObstacleObject* e : obstacles) {
			e->Update(dt);
		}

		SelectObject();
		MoveSelectedObject();
		physics->Update(dt);

		if (lockedObject != nullptr) {
			Vector3 objPos = lockedObject->GetTransform().GetPosition();
			Vector3 camPos = objPos + (lockedObject->GetTransform().GetOrientation() * Vector3(0, min(max(0, lockedPitch), 45), 20));

			Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

			Matrix4 modelMat = temp.Inverse();

			Quaternion q(modelMat);
			Vector3 angles = q.ToEuler(); //nearly there now!

			world->GetMainCamera()->SetPosition(camPos);
			world->GetMainCamera()->SetPitch(angles.x);
			world->GetMainCamera()->SetYaw(angles.y);

			//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
		}

		if (elapsedTime <= 0 && player != nullptr) {
			elapsedTime = 1.0f;
			player->DecrementScore();
			for (GameObject* e : enemies) {
				e->DecrementScore();
			}
		}
		
		renderer->DrawString("Current Score: " + std::to_string(player->GetScore()), Vector2(5, 80));
		

		world->UpdateWorld(dt);
		if (WinConditionMet(player)) {
			winnerName = player->GetName();
		}
		else {
			for (GameObject* e : enemies) {
				if (WinConditionMet(e)) {
					winnerName = e->GetName();
				}
			}
		}

		if (player->GetTransform().GetPosition().y < -5) {
			player->GetTransform().SetPosition(Vector3(-20, 5, 20));
		}

		renderer->Update(dt);
	}


	Debug::FlushRenderables(dt);
	renderer->Render();
}

void CourseworkGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
		lockedObject = nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}
	//Always use gravity
	physics->UseGravity(true);
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F6)) {
		std::cout << player->GetTransform().GetPosition();
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void CourseworkGame::LockedObjectMovement() {
	Matrix4 view = world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld = view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	float force = 50;

	//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::A)) {
	//	lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	//}

	//if (Window::GetKeyboard()->KeyDown(KeyboardKeys::D)) {
	//	//Vector3 worldPos = selectionObject->GetTransform().GetPosition();
	//	lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	//}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::W)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::S)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::A)) {
		lockedObject->GetPhysicsObject()->AddForceAtPosition(fwdAxis * 10, lockedObject->GetTransform().GetPosition() + lockedObject->GetTransform().GetOrientation() * Vector3(1, 0, 0));
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::D)) {
		lockedObject->GetPhysicsObject()->AddForceAtPosition(fwdAxis * 10, lockedObject->GetTransform().GetPosition() - lockedObject->GetTransform().GetOrientation() * Vector3(1, 0, 0));
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::SPACE) && lockedObject->IsGrounded()) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,1000,0));
	}
}

void CourseworkGame::DebugObjectMovement() {
	//If we've selected an object, we can manipulate it with some key presses
	/*if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}
	*/
}

void CourseworkGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void CourseworkGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	InitPlayer();
	if (multi) {
		InitEnemies();
	}
	InitCoins();
	InitDefaultFloor();
	BridgeConstraintTest();
	AddWallsToWorld(Vector3(0, -2, 0));
	AddStateObstacleToWorld(Vector3(5, 5, 5));
}

void CourseworkGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(1, 0.1, 2);
	float invCubeMass = 5;
	int numLinks = 27;
	float maxDistance = 1.5;
	float cubeDistance = 2;

	Vector3 startPos = Vector3(-29, -1, -35);
	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);
	GameObject* previous = start;

	for (int i = 0; i < numLinks; i++) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
vector<GameObject*> CourseworkGame::AddWallsToWorld(const Vector3& position) {
	vector<GameObject*> walls;

	GameObject* wall1 = new GameObject("Wall");
	Vector3 wallSize1 = Vector3(40, 4, 4);
	AABBVolume* volume1 = new AABBVolume(wallSize1);
	wall1->SetBoundingVolume((CollisionVolume*)volume1);
	wall1->GetTransform()
		.SetScale(wallSize1 * 2)
		.SetPosition(position + Vector3(-10, 6, 0));

	wall1->SetRenderObject(new RenderObject(&wall1->GetTransform(), cubeMesh, basicTex, basicShader));
	wall1->SetPhysicsObject(new PhysicsObject(&wall1->GetTransform(), wall1->GetBoundingVolume()));

	wall1->GetPhysicsObject()->SetInverseMass(0);
	wall1->GetPhysicsObject()->SetFriction(1);
	wall1->GetPhysicsObject()->SetElasticity(0.5);
	wall1->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall1);
	walls.push_back(wall1);

	GameObject* wall2 = new GameObject("Wall");
	Vector3 wallSize2 = Vector3(40, 4, 4);
	AABBVolume* volume2 = new AABBVolume(wallSize2);
	wall2->SetBoundingVolume((CollisionVolume*)volume2);
	wall2->GetTransform()
		.SetScale(wallSize2 * 2)
		.SetPosition(Vector3(10, 4, -23));

	wall2->SetRenderObject(new RenderObject(&wall2->GetTransform(), cubeMesh, basicTex, basicShader));
	wall2->SetPhysicsObject(new PhysicsObject(&wall2->GetTransform(), wall2->GetBoundingVolume()));

	wall2->GetPhysicsObject()->SetInverseMass(0);
	wall2->GetPhysicsObject()->SetFriction(1);
	wall2->GetPhysicsObject()->SetElasticity(0.5);
	wall2->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall2);
	walls.push_back(wall2);

	GameObject* wall3 = new GameObject("Wall");
	Vector3 wallSize3 = Vector3(40, 4, 4);
	AABBVolume* volume3 = new AABBVolume(wallSize3);
	wall3->SetBoundingVolume((CollisionVolume*)volume3);
	wall3->GetTransform()
		.SetScale(wallSize3 * 2)
		.SetPosition(position + Vector3(-10, 6, -45));

	wall3->SetRenderObject(new RenderObject(&wall3->GetTransform(), cubeMesh, basicTex, basicShader));
	wall3->SetPhysicsObject(new PhysicsObject(&wall3->GetTransform(), wall3->GetBoundingVolume()));

	wall3->GetPhysicsObject()->SetInverseMass(0);
	wall3->GetPhysicsObject()->SetFriction(1);
	wall3->GetPhysicsObject()->SetElasticity(0.5);
	wall3->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall3);
	walls.push_back(wall3);

	GameObject* wall4 = new GameObject("Wall");
	Vector3 wallSize4 = Vector3(40, 4, 4);
	AABBVolume* volume4 = new AABBVolume(wallSize4);
	wall4->SetBoundingVolume((CollisionVolume*)volume4);
	wall4->GetTransform()
		.SetScale(wallSize4 * 2)
		.SetPosition(position + Vector3(10, 6, -70));

	wall4->SetRenderObject(new RenderObject(&wall4->GetTransform(), cubeMesh, basicTex, basicShader));
	wall4->SetPhysicsObject(new PhysicsObject(&wall4->GetTransform(), wall4->GetBoundingVolume()));

	wall4->GetPhysicsObject()->SetInverseMass(0);
	wall4->GetPhysicsObject()->SetFriction(1);
	wall4->GetPhysicsObject()->SetElasticity(0.5);
	wall4->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall4);
	walls.push_back(wall4);

	GameObject* wall5 = new GameObject("Wall");
	Vector3 wallSize5 = Vector3(50, 4, 4);
	AABBVolume* volume5 = new AABBVolume(wallSize5);
	wall5->SetBoundingVolume((CollisionVolume*)volume5);
	wall5->GetTransform()
		.SetScale(wallSize5 * 2)
		.SetPosition(Vector3(0, 4, 29));

	wall5->SetRenderObject(new RenderObject(&wall5->GetTransform(), cubeMesh, basicTex, basicShader));
	wall5->SetPhysicsObject(new PhysicsObject(&wall5->GetTransform(), wall5->GetBoundingVolume()));

	wall5->GetPhysicsObject()->SetInverseMass(0);
	wall5->GetPhysicsObject()->SetFriction(1);
	wall5->GetPhysicsObject()->SetElasticity(0.5);
	wall5->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall5);
	walls.push_back(wall5);

	GameObject* wall6 = new GameObject("Wall");
	Vector3 wallSize6 = Vector3(50, 4, 4);
	AABBVolume* volume6 = new AABBVolume(wallSize6);
	wall6->SetBoundingVolume((CollisionVolume*)volume6);
	wall6->GetTransform()
		.SetScale(wallSize6 * 2)
		.SetPosition(Vector3(0, 4, -99));

	wall6->SetRenderObject(new RenderObject(&wall6->GetTransform(), cubeMesh, basicTex, basicShader));
	wall6->SetPhysicsObject(new PhysicsObject(&wall6->GetTransform(), wall6->GetBoundingVolume()));

	wall6->GetPhysicsObject()->SetInverseMass(0);
	wall6->GetPhysicsObject()->SetFriction(1);
	wall6->GetPhysicsObject()->SetElasticity(0.5);
	wall6->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall6);
	walls.push_back(wall6);

	GameObject* wall7 = new GameObject("Wall");
	Vector3 wallSize7 = Vector3(4, 4, 60);
	AABBVolume* volume7 = new AABBVolume(wallSize7);
	wall7->SetBoundingVolume((CollisionVolume*)volume7);
	wall7->GetTransform()
		.SetScale(wallSize7 * 2)
		.SetPosition(Vector3(54, 4, -60 + 25));

	wall7->SetRenderObject(new RenderObject(&wall7->GetTransform(), cubeMesh, basicTex, basicShader));
	wall7->SetPhysicsObject(new PhysicsObject(&wall7->GetTransform(), wall7->GetBoundingVolume()));

	wall7->GetPhysicsObject()->SetInverseMass(0);
	wall7->GetPhysicsObject()->SetFriction(1);
	wall7->GetPhysicsObject()->SetElasticity(0.5);
	wall7->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall7);
	walls.push_back(wall7);

	GameObject* wall8 = new GameObject("Wall");
	Vector3 wallSize8 = Vector3(4, 4, 60);
	AABBVolume* volume8 = new AABBVolume(wallSize8);
	wall8->SetBoundingVolume((CollisionVolume*)volume8);
	wall8->GetTransform()
		.SetScale(wallSize8 * 2)
		.SetPosition(Vector3(-54, 4, -60 + 25));

	wall8->SetRenderObject(new RenderObject(&wall8->GetTransform(), cubeMesh, basicTex, basicShader));
	wall8->SetPhysicsObject(new PhysicsObject(&wall8->GetTransform(), wall8->GetBoundingVolume()));

	wall8->GetPhysicsObject()->SetInverseMass(0);
	wall8->GetPhysicsObject()->SetFriction(1);
	wall8->GetPhysicsObject()->SetElasticity(0.5);
	wall8->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall8);
	walls.push_back(wall8);

	return walls;
}

vector<GameObject*> CourseworkGame::AddFloorToWorld(const Vector3& position) {
	vector<GameObject*> floors;
	
	GameObject* floor = new GameObject("World");

	Vector3 floorSize = Vector3(50, 2, 25);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->SetFriction(1);
	floor->GetPhysicsObject()->SetElasticity(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);
	floors.push_back(floor);

	GameObject* floor2 = new GameObject("World");

	Vector3 floorSize2 = Vector3(10, 2, 10);
	AABBVolume* volume2 = new AABBVolume(floorSize2);
	floor2->SetBoundingVolume((CollisionVolume*)volume2);
	floor2->GetTransform()
		.SetScale(floorSize2 * 2)
		.SetPosition(position + Vector3(-40, 0, -35));

	floor2->SetRenderObject(new RenderObject(&floor2->GetTransform(), cubeMesh, basicTex, basicShader));
	floor2->SetPhysicsObject(new PhysicsObject(&floor2->GetTransform(), floor2->GetBoundingVolume()));

	floor2->GetPhysicsObject()->SetInverseMass(0);
	floor2->GetPhysicsObject()->SetFriction(1);
	floor2->GetPhysicsObject()->SetElasticity(0);
	floor2->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor2);
	floors.push_back(floor2);

	GameObject* floor3 = new GameObject("World");

	Vector3 floorSize3 = Vector3(10, 2, 10);
	AABBVolume* volume3 = new AABBVolume(floorSize3);
	floor3->SetBoundingVolume((CollisionVolume*)volume3);
	floor3->GetTransform()
		.SetScale(floorSize3 * 2)
		.SetPosition(position + Vector3(40, 0, -35));

	floor3->SetRenderObject(new RenderObject(&floor3->GetTransform(), cubeMesh, basicTex, basicShader));
	floor3->SetPhysicsObject(new PhysicsObject(&floor3->GetTransform(), floor3->GetBoundingVolume()));

	floor3->GetPhysicsObject()->SetInverseMass(0);
	floor3->GetPhysicsObject()->SetFriction(1);
	floor3->GetPhysicsObject()->SetElasticity(0);
	floor3->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor3);
	floors.push_back(floor3);

	GameObject* floor4 = new GameObject("World");
	
	Vector3 floorSize4 = Vector3(50, 2, 25);
	AABBVolume* volume4 = new AABBVolume(floorSize4);
	floor4->SetBoundingVolume((CollisionVolume*)volume4);
	floor4->GetTransform()
		.SetScale(floorSize4 * 2)
		.SetPosition(position + Vector3(0, 0, -70));

	floor4->SetRenderObject(new RenderObject(&floor4->GetTransform(), cubeMesh, basicTex, basicShader));
	floor4->SetPhysicsObject(new PhysicsObject(&floor4->GetTransform(), floor4->GetBoundingVolume()));

	floor4->GetPhysicsObject()->SetInverseMass(0);
	floor4->GetPhysicsObject()->SetFriction(1);
	floor4->GetPhysicsObject()->SetElasticity(0);
	floor4->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor4);
	floors.push_back(floor4);

	return floors;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple'
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* CourseworkGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* CourseworkGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius * 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}

GameObject* CourseworkGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject("World");

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

void CourseworkGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void CourseworkGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void CourseworkGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols + 1; ++x) {
		for (int z = 1; z < numRows + 1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void CourseworkGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}

void CourseworkGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

void CourseworkGame::InitPlayer()
{
	player = AddPlayerToWorld(Vector3(0,20,0));
	player->GetPhysicsObject()->SetElasticity(0);
	player->GetPhysicsObject()->SetFriction(1);
	lockedObject = player;
}

void CourseworkGame::InitEnemies() {
	for (int i = 0; i < 4; i++) {
		StateGameObject* enemy = AddStateEnemyToWorld(Vector3(-30, 5, 30));
		enemy->GetPhysicsObject()->SetElasticity(0);
		enemy->GetPhysicsObject()->SetFriction(1);
		enemies.emplace_back(enemy);
		enemy->GetPhysicsObject()->SetElasticity(0);
	}
}

void CourseworkGame::InitCoins() {
	for (int i = 0; i < 20; i++) {
		GameObject* coin = AddBonusToWorld(Vector3(i * rand() % 10, 5, i * rand() % 10));
		coin->GetPhysicsObject()->SetElasticity(0);
		coin->GetPhysicsObject()->SetFriction(1);
		coin->GetPhysicsObject()->SetInverseMass(0);
	}
}

GameObject* CourseworkGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject("Player");

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(Vector3(-20, 5, 20));

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* CourseworkGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject("Enemy");

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* CourseworkGame::AddBonusToWorld(const Vector3& position) {
	GameObject* coin = new GameObject("Coin");

	SphereVolume* volume = new SphereVolume(0.25f);
	coin->SetBoundingVolume((CollisionVolume*)volume);
	coin->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	coin->SetRenderObject(new RenderObject(&coin->GetTransform(), bonusMesh, nullptr, basicShader));
	coin->SetPhysicsObject(new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume()));

	coin->GetPhysicsObject()->SetInverseMass(1.0f);
	coin->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(coin);

	return coin;
}

StateGameObject* CourseworkGame::AddStateEnemyToWorld(const Vector3& position)
{
	float meshSize = 3.0f;
	float inverseMass = 0.5f;
	StateGameObject* enemy = new StateGameObject("Enemy");

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);

	enemy->SetBoundingVolume((CollisionVolume*)volume);
	enemy->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(Vector3(-20, 5, 20));

	enemy->SetRenderObject(new RenderObject(&enemy->GetTransform(), enemyMesh, nullptr, basicShader));
	enemy->SetPhysicsObject(new PhysicsObject(&enemy->GetTransform(), enemy->GetBoundingVolume()));

	enemy->GetPhysicsObject()->SetInverseMass(inverseMass);
	enemy->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(enemy);

	return enemy;
}

StateObstacleObject* CourseworkGame::AddStateObstacleToWorld(const Vector3& position)
{
	float inverseMass = 0;
	StateObstacleObject* obstacle = new StateObstacleObject("Enemy");

	AABBVolume* volume = new AABBVolume(Vector3(3, 3, 3));

	obstacle->SetBoundingVolume((CollisionVolume*)volume);
	obstacle->GetTransform()
		.SetPosition(position)
		.SetScale(Vector3(3, 3, 3) * 2);

	obstacle->SetRenderObject(new RenderObject(&obstacle->GetTransform(), cubeMesh, nullptr, basicShader));
	obstacle->SetPhysicsObject(new PhysicsObject(&obstacle->GetTransform(), obstacle->GetBoundingVolume()));

	obstacle->GetPhysicsObject()->SetInverseMass(inverseMass);
	obstacle->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(obstacle);

	obstacles.push_back(obstacle);

	return obstacle;
}

bool CourseworkGame::WinConditionMet(GameObject* testSubject)
{
	//std::cout << testSubject->GetName() << std::endl;
	//if player in win area
	if (testSubject->GetTransform().GetPosition().x > 25 && testSubject->GetTransform().GetPosition().z < -70 &&
		testSubject->GetTransform().GetPosition().x < 50 && testSubject->GetTransform().GetPosition().z > -95 && 
		testSubject->GetTransform().GetPosition().y > 0 && testSubject->GetScore() > 0) {
		//win
		return true;
	}
	return false;
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around.

*/
bool CourseworkGame::SelectObject() {
	/*if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
				lockedObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}*/

	if (lockedObject) {
		//renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if (selectionObject) {
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

	}

	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void CourseworkGame::MoveSelectedObject() {
	if (!selectionObject) return;

	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * 10, closestCollision.collidedAt);
			}
		}
	}
}