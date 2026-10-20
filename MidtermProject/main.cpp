#include <windows.h>
#include <vector>
#include <string>
#include <random>
#include "GameObject.h"
#include "Brick.h"
#include "Ball.h"

#define SPEED 200.0f
#define PI 3.14159265f


class MyApp
{
public:
	MyApp() {};
	~MyApp() { DiscardDeviceResource(); DiscardAppResource(); };
	bool Initialize(HINSTANCE hInstance);
	bool Update(float timeDelta);
	void Render();
	void RenderMainMenu(HDC hdcMem);

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnResize();
	bool CreateAppResource();
	void DiscardAppResource();
	bool CreateDeviceResource();
	void DiscardDeviceResource();
	void InitGameObject();
	void ResetGame();
	CollisionType HandleCollision(RECT ball, RECT obj);

private:
	HWND hWnd = NULL;
	float timeDuration;
	float alpha;

	// 더블 버퍼링을 위한 멤버
	HDC hdcMem = NULL;
	HBITMAP hBitmapMem = NULL;

	// GameObject
	vector<GameObject*> walls;
	vector<Brick*> bricks;
	vector<Ball*> balls;
	GameObject* paddle = NULL;

	// Game Screen Value
	RECT clientRect;
	POINT mousePos;
	RECT GameStartTextBox = { 220, 250, 420, 290 };
	RECT ExitTextBox = { 220, 300, 420, 340 };
	bool mainMenu = true;
	bool ready = true;
	bool inGame = false;
	bool gameOver = false;
	bool gameClear = false;
	int gameClearCount;
	int stage = 0;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MyApp myApp;
	if (!myApp.Initialize(hInstance)) return 1;

	// 시간 변수 설정.
	//DeltaTime을 계산하는데 사용. ( CurrentTime - Prev Time ) / Frequency
	LARGE_INTEGER nPrevTime;
	LARGE_INTEGER nFrequency;
	// Time 초기화
	QueryPerformanceFrequency(&nFrequency);
	QueryPerformanceCounter(&nPrevTime);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			LARGE_INTEGER nCurrentTime;
			QueryPerformanceCounter(&nCurrentTime);
			float timeDelta = (float)((double)(nCurrentTime.QuadPart - nPrevTime.QuadPart) / (double)(nFrequency.QuadPart));
			nPrevTime = nCurrentTime;

			if (!myApp.Update(timeDelta)) break;

			myApp.Render();
		}
	}
	return 0;
}

bool MyApp::Initialize(HINSTANCE hInstance)
{
	// Initialize Window Class
	WNDCLASS wc = {};
	wc.lpfnWndProc = MyApp::WndProc;
	wc.cbWndExtra = sizeof(LONG_PTR);
	wc.hInstance = hInstance;
	wc.lpszClassName = L"MyApp";
	RegisterClass(&wc);

	hWnd = CreateWindow(L"MyApp", L"MyApp", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, this);
	if (!hWnd) return false;

	GetClientRect(hWnd, &clientRect);
	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	if (!CreateAppResource()) return false;

	InitGameObject();
	
	return true;
}

LRESULT CALLBACK MyApp::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CREATE)
	{
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		MyApp* pMyApp = (MyApp*)pCreate->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pMyApp);
		return 0;
	}

	MyApp* pMyApp = (MyApp*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!pMyApp)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_SIZE:
	{
		pMyApp->OnResize();
		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_KEYDOWN:
	{
		if (pMyApp->ready)
		{
			std::random_device rd;         
			std::mt19937 gen(rd());

			std::uniform_real_distribution<float> distX(-1.0f, 1.0f);
			std::uniform_real_distribution<float> distY(0.1f, 1.0f);
			float randX = distX(gen);
			float randY = distY(gen);
			

			pMyApp->balls[0]->SetBallSpeed(randX * SPEED, -randY * SPEED);
			pMyApp->ready = false;
		}
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		if (pMyApp->gameOver) return 0;
		// Main Menu
		if (pMyApp->mainMenu)
		{
			POINT cursor;
			GetCursorPos(&cursor);
			ScreenToClient(hWnd, &cursor);

			pMyApp->mousePos = cursor;
		}
		// In-Game
		else
		{
			POINT cursor;
			GetCursorPos(&cursor);
			ScreenToClient(hWnd, &cursor);

			if (cursor.x < 50 || cursor.x > 570) return 0;
			if(pMyApp->paddle) pMyApp->paddle->SetPosition(cursor.x-30, pMyApp->clientRect.bottom - 20);
		}
		return 0;
	}
	case WM_LBUTTONUP:
	{
		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(hWnd, &cursor);
		if (pMyApp->mainMenu)
		{
			if (PtInRect(&pMyApp->GameStartTextBox, cursor))
			{
				pMyApp->mainMenu = false;
				pMyApp->ready = true;          // Click to Start
			}
			else if (PtInRect(&pMyApp->ExitTextBox, cursor))
			{
				DestroyWindow(hWnd);
			}
		}
		else if (pMyApp->gameOver || pMyApp->gameClear)
		{
			pMyApp->ResetGame();
			pMyApp->mainMenu = true;
		}
	}
		return 0;

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void MyApp::OnResize()
{
}

bool MyApp::CreateAppResource()
{
	timeDuration = 3.0f;
	alpha = 0;

	// GDI 더블 버퍼링 구현. hdcMem, hBitmapMem 준비
	// 현재 Window 사이즈에 맞는 Bitmap을 생성
	HDC hdc = GetDC(hWnd);
	hdcMem = CreateCompatibleDC(hdc);
	RECT rc;
	GetClientRect(hWnd, &rc);
	hBitmapMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
	ReleaseDC(hWnd, hdc);

	//비트맵 객체를 선택함. 바뀌지 않으므로 한번만 수행함.
	//DC가 그리는 대상은 Bitmap으로 고정되어 있음. Render에서 이를 수행하고 무효 영역 발생 시 Swap
	SelectObject(hdcMem, hBitmapMem);  

	return true;
}

void MyApp::DiscardAppResource()
{
	DeleteObject(hBitmapMem);
	DeleteDC(hdcMem);
}

bool MyApp::CreateDeviceResource()
{
	return true;
}

void MyApp::DiscardDeviceResource()
{
}

CollisionType MyApp::HandleCollision(RECT ball, RECT obj)
{
	RECT area;
	if(IntersectRect(&area, &ball, &obj))
		return ((area.left - area.right) > (area.top - area.bottom) ? HORIZON : VERTICAL);

	return NONE;
}

bool MyApp::Update(float timeDelta)
{
	CollisionType colRes;

	static float timeElapsed = 0;
	timeElapsed += timeDelta;

	if (timeElapsed > timeDuration) timeElapsed = 0.0f;
	alpha = timeElapsed / timeDuration;

	if (mainMenu || gameOver || gameClear) return true;
	if (timeDelta > 0.033f) timeDelta = 0.033f;

	paddle->Update(timeDelta);

	for (auto* obj : balls) obj->Update(timeDelta);

	int brokenBlock = 0;
	for (auto* obj : bricks)
	{
		if (obj->GetIsActive())
		{
			obj->Update(timeDelta);
			colRes = HandleCollision(balls[0]->ObjectRect, obj->ObjectRect);
			if (colRes != NONE)
			{
				balls[0]->OnHit(colRes);
				obj->OnHit(colRes);
			}
		}
		else
		{
			brokenBlock++;
			if (brokenBlock == gameClearCount)
			{
				gameClear = true;
			}
		}
	}
	for (auto* obj : walls)
	{
		obj->Update(timeDelta);
		colRes = HandleCollision(balls[0]->ObjectRect, obj->ObjectRect);
		if (colRes != NONE)
		{
			balls[0]->OnHit(colRes);
			obj->OnHit(colRes);
		}
	}
	colRes = HandleCollision(balls[0]->ObjectRect, paddle->ObjectRect);
	if (colRes != NONE)
	{
		balls[0]->OnHit(colRes);
		paddle->OnHit(colRes);
	}

	if (balls[0]->GetPosition().y > clientRect.bottom)
		gameOver = true;

	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	// 클라이언트 창 hWnd를 얻어오고, 
	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_BACKGROUND));

	// Render Main Menu
	if (mainMenu)
	{
		RenderMainMenu(hdcMem);
	}
	else
	{
		for (auto* object : walls) object->Render(hdcMem);
		for (auto* object : bricks) object->Render(hdcMem);
		for (auto* object : balls) object->Render(hdcMem);
		paddle->Render(hdcMem);

		SetBkMode(hdcMem, TRANSPARENT);
		SetTextColor(hdcMem, RGB(244, 244, 244));

		if (ready)
		{
			DrawText(hdcMem, L"Press Any Button to Start", -1, &rc,
				DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		else if (gameOver)
		{
			RECT upper = rc, lower = rc;
			upper.bottom = (rc.top + rc.bottom) / 2;
			lower.top = upper.bottom;
			DrawText(hdcMem, L"Game Over", -1, &upper,
				DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
			DrawText(hdcMem, L"Click to Menu", -1, &lower,
				DT_CENTER | DT_TOP | DT_SINGLELINE);
		}
		else if (gameClear)
		{
			RECT upper = rc, lower = rc;
			upper.bottom = (rc.top + rc.bottom) / 2;
			lower.top = upper.bottom;
			DrawText(hdcMem, L"Game Clear", -1, &upper,
				DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
			DrawText(hdcMem, L"Click to Menu", -1, &lower,
				DT_CENTER | DT_TOP | DT_SINGLELINE);
		}
	}

	//버퍼 스와핑
	HDC hdc = GetDC(hWnd);
	BitBlt(hdc, 0, 0, rc.right, rc.bottom,hdcMem, 0, 0, SRCCOPY);

	ReleaseDC(hWnd, hdc);
	// 그리기 코드 끝

	bool DeviceLost = false;
	if (DeviceLost) DiscardDeviceResource();
}

void MyApp::RenderMainMenu(HDC hdcMem)
{
	SetTextColor(hdcMem, RGB(244, 244, 244));
	SetBkMode(hdcMem, TRANSPARENT);
	const TCHAR tGameName[] = L"Break Brick!";
	const TCHAR tGameStart[] = L"Game Start";
	const TCHAR	tExit[] = L"Exit";
	

	FillRect(hdcMem, &GameStartTextBox,
		GetSysColorBrush(PtInRect(&GameStartTextBox, mousePos) ? COLOR_BTNSHADOW : COLOR_BTNFACE));
	FillRect(hdcMem, &ExitTextBox,
		GetSysColorBrush(PtInRect(&ExitTextBox, mousePos) ? COLOR_BTNSHADOW : COLOR_BTNFACE));

	TextOut(hdcMem, (clientRect.right - clientRect.left) / 2 - 40, (clientRect.bottom - clientRect.top) / 2 - 100, tGameName, lstrlen(tGameName));
	SetTextColor(hdcMem, RGB(0, 0, 0));
	DrawText(hdcMem, L"Game Start", -1, &GameStartTextBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	DrawText(hdcMem, L"Exit", -1, &ExitTextBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void MyApp::InitGameObject()
{
	// Initialize Walls
	walls.push_back(new GameObject("Left wall",  clientRect.left + 10,  clientRect.top + 10, 10, 470));		//왼쪽
	walls.push_back(new GameObject("Upper wall", clientRect.left + 10,  clientRect.top + 10, 600,  10));	//위
	walls.push_back(new GameObject("Right wall", clientRect.left + 600, clientRect.top + 10, 10, 470));		//오른쪽

	// Initialize balls
	balls.push_back(new Ball("Player Ball", (clientRect.right - clientRect.left)/2 - 10, clientRect.bottom - 50, 10, 10, 0, 0));

	// Initialize Bricks
	for (int row = clientRect.top; row < 7; row++)
	{
		for (int column = clientRect.left; column < 9; column++)
		{
			bricks.push_back(new Brick("B" + to_string(5 * row + column), 40 + column * 60, 25 + row * 25, 55, 20, true));
		}
	}

	// initialize paddle
	paddle = new GameObject("Player Paddle", (clientRect.right - clientRect.left) / 2 - 30, clientRect.bottom - 20, 60, 10);

	gameClearCount = bricks.size();
}

void MyApp::ResetGame()
{
	for (auto* b : bricks) b->SetIsActive(true);
	paddle->SetPosition((clientRect.right - clientRect.left) / 2 - 30, clientRect.bottom - 20);
	balls[0]->SetBallSpeed(0.0f, 0.0f);
	balls[0]->SetPosition((clientRect.right - clientRect.left) / 2 - 10, clientRect.bottom - 50);
	gameOver = false;
	ready = true;
	gameClear = false;
}