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
	//AI jumps if no path available, of if it thinks it is blocked to jump over the blockade
	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()->bool { //pathfind when available
		TestForPath();
		return pathFound == true || GetPhysicsObject()->GetLinearVelocity().Length() <= 2;
		
		}));
	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()->bool { //jump if blocked
		TestForPath();
		return pathFound == false || GetPhysicsObject()->GetLinearVelocity().Length() > 2;
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
	Vector3 shift(55, 0, 100);
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
	if (GetPhysicsObject()->GetLinearVelocity().Length() <= 2) {
		if (grounded) {
			//GetPhysicsObject()->AddForce({ 0, 1000, 0 });//Jump if possibly stuck
		}
	}
	Vector3 target = pathNodes[2];
	Vector3 facingNormal = (GetTransform().GetPosition()) - GetTransform().GetOrientation() * Vector3(0, 0, 1);
	GetPhysicsObject()->AddForceAtPosition((target - GetTransform().GetPosition()).Normalised() * 50, facingNormal);

	//Oh yeah look at em goooooo skkrrrrr
	
}

void StateGameObject::Jump(float dt) {
	if (grounded) {
		GetPhysicsObject()->AddForce({ 0, 1000, 0 });
	}
}