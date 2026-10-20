#include "Brick.h"

void Brick::Update(float deltaTime)
{
	GameObject::Update(deltaTime);
}

void Brick::Render(HDC hdcMem)
{
	if (!isActive) return;
	FillRect(hdcMem, &ObjectRect, GetSysColorBrush(RGB(234,234,234)));
}

void Brick::OnHit(CollisionType colRes)
{
	if(colRes != NONE)
		isActive = false;
}