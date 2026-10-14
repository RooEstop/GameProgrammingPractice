#pragma once
#include <vector>
#include <string>
using namespace std;

struct Vec2 { float x, y; };

class GameObject
{
public:
	GameObject(string n, float x, float y, float width, float height) { name = n; pos = { x ,y }; size = { width, height }; };
	virtual ~GameObject();

	vector<float> GetPosition() { return pos; };
	vector<float> GetSize() { return size; };

private:
	string* name = {};
	vector<float> pos = { 0.0f, 0.0f };
	vector<float> size = { 0.0f, 0.0f };
};
