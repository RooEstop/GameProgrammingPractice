#pragma once
#include "GameObject.h"

class Brick : public GameObject
{
public:
	Brick(string n, float x, float y, float width, float height, bool Activity)
		: GameObject(n, x, y, width, height), isActive{ Activity } {
	}
	void SetIsActive(bool b) { isActive = b; };
	bool GetIsActive() { return isActive; };

	virtual void Update(float deltaTime) override;
	virtual void Render(HDC hdcMem) override;
	virtual void OnHit(CollisionType colRes) override;

private:
	bool isActive = true;
};

