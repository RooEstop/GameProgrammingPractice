#include <iostream>
#include <queue>

using namespace std;

struct MyStruct
{
	int key;
	char value;
};

struct CompareKey{ bool operator()(const MyStruct s1, const MyStruct s2) const { return s1.key < s2.key; } };
struct CompareValue { bool operator()(const MyStruct s1, const MyStruct s2) const { return s1.value < s2.value; } };

int main()
{
	priority_queue<MyStruct, vector<MyStruct>, CompareKey> pq1;
	
	MyStruct t1{ 3,'B' };
	MyStruct t2{ 1,'D' };
	MyStruct t3{ 2,'A' };
	MyStruct t4{ 4,'C' };

	pq1.push(t1);
	pq1.push(t2);
	pq1.push(t3);
	pq1.push(t4);

	while (!pq1.empty())
	{
		cout << "Key : " << pq1.top().key << " Value : " << pq1.top().value << endl;
		pq1.pop();
	}

	cout << " ====== " << endl;

	priority_queue<MyStruct, vector<MyStruct>, CompareValue> pq2;

	pq2.push(t1);
	pq2.push(t2);
	pq2.push(t3);
	pq2.push(t4);

	while (!pq2.empty())
	{
		cout << "Key : " << pq2.top().key << " Value : " << pq2.top().value << endl;
		pq2.pop();
	}
}