#include "StateGameObject.h"
#include "StateTransition.h"
#include "State.h"
#include "StateMachine.h"
#include "Debug.h"

using namespace NCL::CSC8503;

StateGameObject::StateGameObject(std::string name) : GameObject(name)
{
	counter = 0.0f;
	grid = new NavigationGrid("TestGrid1.txt");
	pathFound = false;
	stateMachine = new StateMachine();

	State* stateB = new State([&](float dt)->void {
		this->Pathfind(dt);
		});
	State* stateA = new State([&](float dt)->void {
		this->Jump(dt);
		});

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()->bool { //Jump when no path
		TestForPath();
		return pathFound == true;
		
		}));
	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()->bool { //Pathfind if available
		TestForPath();
		return pathFound == false;
		}));
}

StateGameObject::~StateGameObject() {
	delete stateMachine;
}

void StateGameObject::Update(float dt) {
	if(GetTransform().GetPosition().y < -5) {
		GetTransform().SetPosition(Vector3(-20, 5, 20)); //Ressurect if fallen
	}
	stateMachine->Update(dt);
}

bool StateGameObject::TestForPath() {
	pathNodes.clear();
	Vector3 shift(50, 0, 95);
	Vector3 startPos = GetTransform().GetPosition() + shift;
	Vector3 endPos = Vector3(90, 0, 0); //End area
	NavigationPath outPath;
	pathFound = grid->FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		pathNodes.push_back(pos - shift + Vector3(0, 3, 0));
	}
	return pathFound;
}

void StateGameObject::Pathfind(float dt) {

	for (int i = 1; i < pathNodes.size(); i++) {
		Vector3 a = pathNodes[i - 1];
		Vector3 b = pathNodes[i];
		Debug::DrawLine(a, b, Vector4(1, 0, 0, 1));
	}

	Vector3 target = pathNodes[2];
	GetPhysicsObject()->AddForce((target - GetTransform().GetPosition()).Normalised() * 50);
}

void StateGameObject::Jump(float dt) {
	if (grounded) {
		GetPhysicsObject()->AddForce({ 0, 1000, 0 });
	}
}