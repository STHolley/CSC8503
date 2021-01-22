#pragma once
#include "BehaviourNodeWithChildren.h"

class BehaviourSequence : public BehaviourNodeWithChildren {
public:
	BehaviourSequence(std::string name) : BehaviourNodeWithChildren(name) {};
	~BehaviourSequence() {};

	BehaviourState Execute(float dt) override {
		for (auto& i : childNodes) {
			BehaviourState nodeState = i->Execute(dt);
			switch (nodeState) {
			case Success: continue;
			case Failure:
			case Ongoing:
			{
				currentState = nodeState;
				return currentState;
			}
			}
		}
		return Success;
	}
};