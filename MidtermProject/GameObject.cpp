#include "GameObject.h"

void GameObject::Update(float deltaTime)
{
	ObjectRect = { (long)pos.x, (long)pos.y, (long)pos.x + (long)size.x, (long)pos.y + (long)size.y };
}

void GameObject::Render(HDC hdcMem)
{
	RECT rcObject = { pos.x, pos.y, pos.x + size.x, pos.y + size.y };
	FillRect(hdcMem, &rcObject, GetSysColorBrush(COLOR_BTNSHADOW));
}

void GameObject::OnHit(CollisionType)
{

}