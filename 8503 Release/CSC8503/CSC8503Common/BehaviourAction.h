#pragma once
#include "BehaviourNodeWithChildren.h"
#include <functional>

typedef std::function<BehaviourState(float, BehaviourState)> BehaviourActionFunc;

class BehaviourAction : public BehaviourNode {
public:
	BehaviourAction(std::string name, BehaviourActionFunc f) : BehaviourNode(name) {
		function = f;
	};
	~BehaviourAction() {};

	BehaviourState Execute(float dt) override {
		currentState = function(dt, currentState);
		return currentState;
	}
protected:
	BehaviourActionFunc function;
};