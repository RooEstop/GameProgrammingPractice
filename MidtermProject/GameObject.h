#pragma once
#include <windows.h>
#include <vector>
#include <string>

using namespace std;

enum CollisionType {
	NONE,
	HORIZON,
	VERTICAL
};

struct Vec2 { float x, y; };

class GameObject
{
public:
	GameObject(string n, float x, float y, float width, float height) {
		name = n; pos = { x ,y }; size = { width, height }; ObjectRect = { (long)pos.x, (long)pos.y, (long)pos.x + (long)size.x, (long)pos.y + (long)size.y };
	};
	virtual ~GameObject() {};

	void SetPosition(float x, float y) { pos.x = x; pos.y = y; };
	Vec2 GetPosition() { return pos; };

	void SetSize(float x, float y) { size.x = 0.0f; size.y = 0.0f; };
	Vec2 GetSize() { return size; };
	
	virtual void Update(float deltaTime);
	virtual void Render(HDC hdcMem);
	virtual void OnHit(CollisionType colRes);

public:
	RECT ObjectRect;

protected:
	string name = {};
	Vec2 pos = { 0.0f, 0.0f };
	Vec2 size = { 0.0f, 0.0f };
};
