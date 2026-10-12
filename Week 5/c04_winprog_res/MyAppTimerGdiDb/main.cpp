#include <windows.h>

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

	// 예제를 위한 변수들 선언
	// 애니메이션
	float timeDuration; // 애니매이션 지속 시간. 분모로 사용되므로 0이 되면 안됨.
	float alpha; // 애니메이션 동안의 현재 위치값, [0,1] 사이의 값.

	//!{! [더블 버퍼링인 코드]
	// GDI 더블 버퍼링 구현
	HDC hdcMem = NULL;
	HBITMAP hBitmapMem = NULL;
	//!}! 

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MyApp myApp;
	if (!myApp.Initialize(hInstance)) return 1;

	// Time 선언
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
			// Time 갱신
			LARGE_INTEGER nCurrentTime;
			QueryPerformanceCounter(&nCurrentTime);
			float timeDelta = (float)((double)(nCurrentTime.QuadPart - nPrevTime.QuadPart) / (double)(nFrequency.QuadPart));
			nPrevTime = nCurrentTime;

			// 갱신 함수를 호출한다. false가 리턴되면 종료한다.
			if (!myApp.Update(timeDelta)) break;

			myApp.Render();
		}
	}
	return 0;
}

bool MyApp::Initialize(HINSTANCE hInstance)
{
	// 윈도우 클래스를 등록함.
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
	DiscardAppResource();
	CreateAppResource();
}

bool MyApp::CreateAppResource()
{
	// 애니메이션 변수 초기화
	timeDuration = 3.0f;
	alpha = 0;

	//!{! [더블 버퍼링인 코드]
	// GDI 더블 버퍼링 구현. hdcMem, hBitmapMem 준비.
	HDC hdc = GetDC(hWnd);
	hdcMem = CreateCompatibleDC(hdc);
	RECT rc;
	GetClientRect(hWnd, &rc);
	hBitmapMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
	ReleaseDC(hWnd, hdc);

	// 비트맵 객체를 선택함. 바뀌지 않으므로 한 번만 수행함.
	SelectObject(hdcMem, hBitmapMem);
	//!}!

	return true;
}

void MyApp::DiscardAppResource()
{
	//!{! [더블 버퍼링인 코드]
	// GDI 더블 버퍼링 구현. hdcMem, hBitmapMem 소멸.
	DeleteObject(hBitmapMem);
	DeleteDC(hdcMem);
	//!}!
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

	// 응용 프로그램을 종료하는 경우에 false를 리턴하고 그 외에는 항상 true를 리턴함.
	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	/*
	//!{! [더블 버퍼링이 아닌 코드]
	// 그리기 코드 시작
	HDC hdc = GetDC(hWnd);

	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdc, &rc, GetSysColorBrush(COLOR_BACKGROUND));

	int pos = (int)(alpha * rc.right);
	RECT rcBox = { pos, 100, pos + 50, 150 }; //left,top,right,bottom
	FillRect(hdc, &rcBox, GetSysColorBrush(COLOR_HIGHLIGHT));

	ReleaseDC(hWnd, hdc);
	// 그리기 코드 끝
	//!}!
	*/

	//!{! [더블 버퍼링인 코드]
	// 그리기 코드 시작 - 기존의 hdc 대신 hdcMem에 그리면 됨.
	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_BACKGROUND));

	int pos = (int)(alpha * rc.right);
	RECT rcBox = { pos, 100, pos + 50, 150 }; //left,top,right,bottom
	FillRect(hdcMem, &rcBox, GetSysColorBrush(COLOR_HIGHLIGHT));
	// 그리기 코드 끝

	// 버퍼 스와핑.
	HDC hdc = GetDC(hWnd);
	BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
	ReleaseDC(hWnd, hdc);
	//!}!

	bool DeviceLost = false;
	if (DeviceLost) DiscardDeviceResource();
}
