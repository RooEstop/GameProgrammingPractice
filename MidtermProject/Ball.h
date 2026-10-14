#pragma once
#include "GameObject.h"

class Ball : public GameObject
{
public:
	Ball(string n, float x, float y, float width, float height, float vx, float vy)
		: GameObject(n, x, y, width, height) , speed{ vx, vy } {}
	void SetBallSpeed(float vx, float vy) { speed = { vx, vy }; };
	vector<float> GetBallSpeed() { return speed; };

private:
	vector<float> speed = { 0.0f, 0.0f };
};

