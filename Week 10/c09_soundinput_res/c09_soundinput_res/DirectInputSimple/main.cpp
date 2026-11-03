#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <dwrite.h>
#pragma comment(lib, "dwrite")

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8")
#pragma comment(lib, "dxguid") //GUID_*

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// 마우스
#define DIMOUSE_LEFTBUTTON		0
#define DIMOUSE_RIGHTBUTTON		1
#define DIMOUSE_MIDDLEBUTTON	2
#define DIMOUSE_4BUTTON			3
#define DIMOUSE_5BUTTON			4
#define DIMOUSE_6BUTTON			5
#define DIMOUSE_7BUTTON			6
#define DIMOUSE_8BUTTON			7

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
	// DirectInput
	IDirectInput8* pDirectInput = NULL;

	// 키보드

	IDirectInputDevice8* diKeyboard = NULL;
	char keyBuffer[256];

	DWORD key = 0;

	// 마우스

	IDirectInputDevice8* diMouse = NULL;
	DIMOUSESTATE2 mouseState; //마우스 상태.
	// 마우스 성능.
	DWORD mouseNumAxes; //마우스 축의 개수.
	DWORD mouseNumButtons; //마우스 버튼의 개수.
	// 마우스 속성
	float mouseSensitivity; // 마우스 민감도.

	POINT pos; //마우스 커서 위치

	// 마우스 함수
	bool mouseButtonDown(int Button)
	{
		// 마우스 버턴이 눌러졌는지를 확인함.
		return (mouseState.rgbButtons[Button] & 0x80);
	}

	// 마우스 커서의 상대 변이.
	float getRelativeX() { return (float)mouseState.lX * mouseSensitivity; }
	float getRelativeY() { return (float)mouseState.lY * mouseSensitivity; }
	float getRelativeZ() { return (float)mouseState.lZ * mouseSensitivity; }

	// 마우스 커서의 절대위치 관련.
	void getCursorPos(POINT* pos)
	{
		GetCursorPos(pos);
		ScreenToClient(hWnd, pos); //스크린 좌표값을 클라이언트 좌표값으로 바꾼다.
	}
	void setCursorPos(POINT* pos)
	{
		ClientToScreen(hWnd, pos); //클라이언트 영역에서의 좌표값을 스크린 전체 영역에서의 좌표값으로 바꾼다.
		SetCursorPos(pos->x, pos->y);
	}

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
	pDWriteFactory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20.f, L"", &pTextFormat);
	pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	// DirectInput 객체를 생성함.
	DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pDirectInput, NULL);

	// 키보드 장치를 초기화함.
	pDirectInput->CreateDevice(GUID_SysKeyboard, &diKeyboard, NULL);
	diKeyboard->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	diKeyboard->SetDataFormat(&c_dfDIKeyboard);
	diKeyboard->Acquire();

	// 마우스 장치를 초기화함.
	pDirectInput->CreateDevice(GUID_SysMouse, &diMouse, NULL);
	diMouse->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	diMouse->SetDataFormat(&c_dfDIMouse2);
	diMouse->Acquire();

	DIDEVCAPS mouseCapabilities;
	mouseCapabilities.dwSize = sizeof(mouseCapabilities);
	diMouse->GetCapabilities(&mouseCapabilities);
	if (!(mouseCapabilities.dwFlags & DIDC_ATTACHED))
	{
		// 에러 : 현재 시스템에 마우스가 없음
		return false;
	}

	mouseNumAxes = mouseCapabilities.dwAxes;
	mouseNumButtons = mouseCapabilities.dwButtons;
	mouseSensitivity = 1.0f;

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pDWriteFactory);
	SAFE_RELEASE(pTextFormat);

	if (diKeyboard != NULL) diKeyboard->Unacquire();
	SAFE_RELEASE(diKeyboard);

	if (diMouse != NULL) diMouse->Unacquire();
	SAFE_RELEASE(diMouse);

	SAFE_RELEASE(pDirectInput);
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
{
	// 응용 프로그램을 종료하는 경우에 false를 리턴하고 그 외에는 항상 true를 리턴함.

	// 입력 상태를 갱신함.

	// 키보드 장치의 상태를 갱신함.

	HRESULT hr = diKeyboard->GetDeviceState(sizeof(keyBuffer), (void*)&keyBuffer);
	if (FAILED(hr)) return false;

	if (keyBuffer[DIK_RIGHT] & 0x80)	key = 1;
	if (keyBuffer[DIK_LEFT] & 0x80)		key = 2;
	if (keyBuffer[DIK_UP] & 0x80)		key = 3;
	if (keyBuffer[DIK_DOWN] & 0x80)		key = 4;
	if (keyBuffer[DIK_ESCAPE] & 0x80)	return false; // ESC키가 눌리면 종료

	// 마우스 장치의 상태를 갱신함.

	if (diMouse->GetDeviceState(sizeof(mouseState), (void*)&mouseState) == DIERR_INPUTLOST)
	{
		diMouse->Acquire();
	}

	getCursorPos(&pos);

	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// 상태 정보 그리기

	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

	// 키보드 정보 표기하기

	const WCHAR* keyNames[] = { L"DIK_RIGHT", L"DIK_LEFT", L"DIK_UP", L"DIK_DOWN" };
	WCHAR strbuf[180];
	if (key == 0) {
		wsprintf(strbuf, L"화살표 키를 누르세요!\n ...");
	}
	else {
		wsprintf(strbuf, L"화살표 키를 누르세요!\n %s가 입력되었습니다.", keyNames[key - 1]);
	}

	pRenderTarget->DrawText(strbuf, (int)wcslen(strbuf), pTextFormat, D2D1::RectF(0, 0, rtSize.width, rtSize.height / 2), pBlackBrush);

	// 마우스 정보 표기하기

	if (pos.x < 0 || pos.x >= (long)rtSize.width || pos.y < 0 || pos.y >= (long)rtSize.height)
		wsprintf(strbuf, L"현재의 마우스 위치:\n outside");
	else
		wsprintf(strbuf, L"현재의 마우스 위치:\n [%d, %d]", pos.x, pos.y);

	pRenderTarget->DrawText(strbuf, (int)wcslen(strbuf), pTextFormat, D2D1::RectF(0, rtSize.height / 2, rtSize.width, rtSize.height), pBlackBrush);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
