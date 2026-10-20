#include "Ball.h"

void Ball::Update(float deltaTime)
{
	pos.x += speed.x * deltaTime;
	pos.y += speed.y * deltaTime;
	GameObject::Update(deltaTime);
}

void Ball::Render(HDC hdcMem)
{
	HBRUSH hBallBrush = CreateSolidBrush(RGB(255, 255, 0));
	HGDIOBJ hOldBrush = SelectObject(hdcMem, hBallBrush);

	Ellipse(hdcMem, ObjectRect.left, ObjectRect.top, ObjectRect.right, ObjectRect.bottom);
	
	SelectObject(hdcMem, hOldBrush);
	DeleteObject(hBallBrush);
}

void Ball::OnHit(CollisionType colRes)
{
	switch (colRes)
	{
	case NONE:
	{
		return;
	}
	case HORIZON:
	{
		speed.x *= -1;
		return;
	}
	case VERTICAL:
	{
		speed.y *= -1;
		return;
	}
	}
}