#pragma once
#include "GameObject.h"

class Ball : public GameObject
{
public:
	Ball(string n, float x, float y, float width, float height, float vx, float vy)
		: GameObject(n, x, y, width, height) , speed{ vx, vy } {}

	void SetBallSpeed(float vx, float vy) { speed = { vx, vy }; };
	Vec2 GetBallSpeed() { return speed; };

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdcMem) override;
	virtual void OnHit(CollisionType colRes) override;

protected:
	Vec2 speed = { 0.0f, 0.0f };
};

