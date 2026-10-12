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
	float timeDuration;
	float alpha;

	HDC hdcMem = NULL;
	HBITMAP hBitmapMem = NULL;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MyApp myApp;
	if (!myApp.Initialize(hInstance)) return 1;

	// Time ����
	LARGE_INTEGER nPrevTime;
	LARGE_INTEGER nFrequency;
	// Time �ʱ�ȭ
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
			// Time ����
			LARGE_INTEGER nCurrentTime;
			QueryPerformanceCounter(&nCurrentTime);
			float timeDelta = (float)((double)(nCurrentTime.QuadPart - nPrevTime.QuadPart) / (double)(nFrequency.QuadPart));
			nPrevTime = nCurrentTime;

			// ���� �Լ��� ȣ���Ѵ�. false�� ���ϵǸ� �����Ѵ�.
			if (!myApp.Update(timeDelta)) break;

			myApp.Render();
		}
	}
	return 0;
}

bool MyApp::Initialize(HINSTANCE hInstance)
{
	// ������ Ŭ������ �����.
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

	//GDI 더블 버퍼링 구현. hdcMem, hBitmapMem 준비
	HDC hdc = GetDC(hWnd);
	hdcMem = CreateCompatibleDC(hdc);
	RECT rc;
	GetClientRect(hWnd, &rc);
	hBitmapMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
	ReleaseDC(hWnd, hdc);

	//비트맵 객체를 선택함. 바뀌지 않으므로 한번만 수행함.
	SelectObject(hdcMem, hBitmapMem);

	return true;
}

void MyApp::DiscardAppResource()
{
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

	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_BACKGROUND));

	int pos = (int)(alpha * rc.right);
	RECT rcBox = { pos, 100, pos + 50, 150 }; // left,top,right,bottom
	FillRect(hdcMem, &rcBox, GetSysColorBrush(COLOR_HIGHLIGHT));

	//버퍼 스와핑
	HDC hdc = GetDC(hWnd);
	BitBlt(hdc, 0, 0, rc.right, rc.bottom,hdcMem, 0, 0, SRCCOPY);

	ReleaseDC(hWnd, hdc);
	// 그리기 코드 끝

	bool DeviceLost = false;
	if (DeviceLost) DiscardDeviceResource();
}
