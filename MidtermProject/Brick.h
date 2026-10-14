#pragma once
#include "GameObject.h"

class Brick : public GameObject
{
public:
	void SetIsActive(bool b) { isActive = b; };
	bool GetIsActive() { return isActive; };

private:
	bool isActive = false;
};

