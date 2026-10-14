#include <windows.h>
#include <vector>
#include "GameObject.h"


class MyApp
{
public:
	MyApp() {};
	~MyApp() { DiscardDeviceResource(); DiscardAppResource(); };
	bool Initialize(HINSTANCE hInstance);
	bool Update(float timeDelta);
	void Render();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnResize();
	bool CreateAppResource();
	void DiscardAppResource();
	bool CreateDeviceResource();
	void DiscardDeviceResource();

private:
	HWND hWnd = NULL;
	float timeDuration;
	float alpha;

	// 더블 버퍼링을 위한 멤버
	HDC hdcMem = NULL;
	HBITMAP hBitmapMem = NULL;

	// GameObject 관리용
	vector<GameObject*> bricks;
	vector<GameObject*> balls;
	GameObject paddle;
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

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	if (!CreateAppResource()) return false;
	
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

bool MyApp::Update(float timeDelta)
{
	static float timeElapsed = 0;
	timeElapsed += timeDelta;

	if (timeElapsed > timeDuration) timeElapsed = 0.0f;
	alpha = timeElapsed / timeDuration;



	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	// 클라이언트 창 hWnd를 얻어오고, 
	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_BACKGROUND));

	//버퍼 스와핑
	HDC hdc = GetDC(hWnd);
	BitBlt(hdc, 0, 0, rc.right, rc.bottom,hdcMem, 0, 0, SRCCOPY);

	ReleaseDC(hWnd, hdc);
	// 그리기 코드 끝

	bool DeviceLost = false;
	if (DeviceLost) DiscardDeviceResource();
}
