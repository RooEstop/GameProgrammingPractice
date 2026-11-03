#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <dwrite.h>
#pragma comment(lib, "dwrite")
#include <string> //std::wstring

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p)=NULL; } }

class MyApp
{
public:
	MyApp() {};
	~MyApp() { DiscardDeviceResource(); DiscardAppResource(); };
	bool Initialize(HINSTANCE hInstance);
	void Render();

	// 예제를 위한 함수 선언
	bool Update();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnResize();
	bool CreateAppResource();
	void DiscardAppResource();
	bool CreateDeviceResource();
	void DiscardDeviceResource();

private:
	HWND hWnd = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;

	// 예제를 위한 변수들 선언
	// 붓
	ID2D1SolidColorBrush* pBlackBrush = NULL;
	// DWrite
	IDWriteFactory* pDWriteFactory = NULL;
	IDWriteTextFormat* pTextFormat = NULL;
	// 키보드
	int codeVK = 0;
	int codeAscii = 0;
	// 마우스
	POINT mousePosScreen = { 0 };
	// 출력할 문자열
	std::wstring str;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MyApp myApp;
	if (!myApp.Initialize(hInstance)) return 1;

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
			// 갱신 함수를 호출한다. false가 리턴되면 종료한다.
			if (!myApp.Update()) break;

			myApp.Render();
		}
	}
	return 0;
}

bool MyApp::Initialize(HINSTANCE hInstance)
{
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

	// 키보드 이벤트 처리

	case WM_CHAR:
	{
		pMyApp->codeAscii = (int)wParam;
		return 0;
	}
	case WM_KEYDOWN:
	{
		if (wParam == VK_ESCAPE) // ESC 키를 누르면 종료함.
		{
			DestroyWindow(hWnd);
			return 0;
		}

		pMyApp->codeVK = (int)wParam;
		return 0;
	}

	// 마우스 이벤트 처리

	case WM_MOUSEMOVE:
	{
		GetCursorPos(&pMyApp->mousePosScreen); // 마우스 위치를 받아온다
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		POINT pos;
		pos.x = 300; // 클라이언트 좌표계 기준으로 (300, 300)으로 설정
		pos.y = 300;
		ClientToScreen(hWnd, &pos); // 클라이언트 기준 좌표를 바탕화면 기준으로 변환한다
		SetCursorPos(pos.x, pos.y); // 커서를 윈도우 클라이언트 기준으로 (300, 300)에 위치시킨다
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		POINT pos;
		pos.x = 300; // 스크린 좌표계 기준으로 (300, 300)으로 설정, 윈도우를 벗어날 수 있음
		pos.y = 300;
		SetCursorPos(pos.x, pos.y); // 커서를 스크린 기준으로 (300, 300)에 위치시킨다
		return 0;
	}

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void MyApp::OnResize()
{
	if (!pRenderTarget) return;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pRenderTarget->Resize(size);
}

bool MyApp::CreateAppResource()
{
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);

	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
	pDWriteFactory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.f, L"", &pTextFormat);

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pDWriteFactory);
	SAFE_RELEASE(pTextFormat);
}

bool MyApp::CreateDeviceResource()
{
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush);

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pBlackBrush);
}

bool MyApp::Update()
// !{! Windows 입력 예제 코드. 원본 코드의 위치는 WindowsInputSimple/main.cpp.
{
	// 응용 프로그램을 종료하는 경우에 false를 리턴하고 그 외에는 항상 true를 리턴함.

	// 'q' 키를 누르면 응용 프로그램을 종료함.
	// GetAsyncKeyState 함수는 'q'가 아닌 'Q', 항상 대문자를 사용해야 함.
	if (GetAsyncKeyState('Q') & 0x8000f) return false;

	// 입력 상태를 갱신함.

	WCHAR strbuf[180];

	str.clear();

	// 키보드
 
	// VK
	if (codeVK)
		wsprintf(strbuf, L"[VK]: 0x%04x %c", codeVK, codeVK);
	else
		wsprintf(strbuf, L"[VK]: unpressed");
	str += strbuf;
	str += L"\n";

	// ASCII
	if (codeAscii)
		wsprintf(strbuf, L"[ASCII]: 0x%04x %c", codeAscii, codeAscii);
	else
		wsprintf(strbuf, L"[ASCII]: unpressed");
	str += strbuf;
	str += L"\n\n";

	// GetKeyState
	bool isKeyDown = false;
	bool isKeyToggled = false;
	short keyState = GetKeyState(VK_SPACE);
	if (keyState & 0x8000) // MSB가 1이면, 키가 down임.
		isKeyDown = true;
	if (keyState & 0x0001) // LSB가 1이면, 키가 토글됨.
		isKeyToggled = true;
	wsprintf(strbuf, L"[GetKeyState]: VK_SPACE key: %s, %s",
		(isKeyDown ? L"down" : L"up"),
		(isKeyToggled ? L"toggled" : L"untoggled"));
	str += strbuf;
	str += L"\n";

	// GetAsyncKeyState
	bool isKeyDownCurrently = false;
	bool isKeyPressedAfterPreviousCall = false;
	keyState = GetAsyncKeyState(VK_SPACE);
	if (keyState & 0x8000) // MSB가 1이면, 키가 현재 down 상태임.
		isKeyDownCurrently = true;
	if (keyState & 0x0001) // LSB가 1이면, 직전 GetAsyncKeyState 함수의 호출 이후로 키가 눌러진적 있음.
		isKeyPressedAfterPreviousCall = true;
	wsprintf(strbuf, L"[GetAsyncKeyState]: VK_SPACE key: %s, %s",
		(isKeyDownCurrently ? L"down currently" : L"up currently"),
		(isKeyPressedAfterPreviousCall ? L"pressed after prev call" : L"unpressed after prev call"));
	str += strbuf;
	str += L"\n\n";

	// 마우스

	// 스크린 좌표계로 마우스 위치를 표시한다.
	wsprintf(strbuf, L"[screen coord]: X: %4d, Y: %4d", mousePosScreen.x, mousePosScreen.y);
	str += strbuf;
	str += L"\n";

	// 클라이언트 좌표계로 마우스 위치를 표시한다.
	POINT mousePosClient = mousePosScreen;
	ScreenToClient(hWnd, &mousePosClient); // 해당 클라이언트 기준으로 좌표를 변환한다
	wsprintf(strbuf, L"[client coord]: X: %4d, Y: %4d", mousePosClient.x, mousePosClient.y);
	str += strbuf;
	str += L"\n";

	return true;
}
// !}!

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

	pRenderTarget->DrawText(str.c_str(), lstrlen(str.c_str()), pTextFormat, D2D1::RectF(50, 0, rtSize.width, rtSize.height), pBlackBrush);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
