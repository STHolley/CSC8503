#include "StateObstacleObject.h"
#include "StateTransition.h"
#include "State.h"
#include "StateMachine.h"

using namespace NCL::CSC8503;

StateObstacleObject::StateObstacleObject(std::string name, float counter) : GameObject(name)
{
	this->counter = counter;

	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)->void {
		this->MoveUp(dt);
		});
	State* stateB = new State([&](float dt)->void {
		this->MoveDown(dt);
		});

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB, [&]()->bool {
		return this->counter > 3.0f;
		}));
	stateMachine->AddTransition(new StateTransition(stateB, stateA, [&]()->bool {
		return this->counter < 0.0f;
		}));
}

StateObstacleObject::~StateObstacleObject() {
	delete stateMachine;
}

void StateObstacleObject::Update(float dt) {
	stateMachine->Update(dt);
}

void StateObstacleObject::MoveUp(float dt) {
	GetPhysicsObject()->AddForce({ 0, 0, 10 });
	counter += dt;
}

void StateObstacleObject::MoveDown(float dt) {
	GetPhysicsObject()->AddForce({ 0, 0, -10 });
	counter -= dt;
}

