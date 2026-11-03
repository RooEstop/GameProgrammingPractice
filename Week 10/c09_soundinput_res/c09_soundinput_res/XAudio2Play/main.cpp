#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <dwrite.h>
#pragma comment(lib, "dwrite")

#include "XAudio2Helper.h"

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
	// XAudio2로 소리를 재생함
	IXAudio2* pXAudio2 = NULL;
	IXAudio2MasteringVoice* pMasteringVoice = NULL;
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

	// XAudio2 객체를 생성함.

	// XAudio2 초기화.
	UINT32 flags = 0;
	HRESULT hr = XAudio2Create(&pXAudio2, flags);
	if (FAILED(hr)) return false;

	// 마스터링 보이스 생성.
	hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice);
	if (FAILED(hr)) return false;
	
	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pDWriteFactory);
	SAFE_RELEASE(pTextFormat);
	SAFE_RELEASE(pXAudio2);
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

	HRESULT hr = S_OK;

	if (GetAsyncKeyState('1') & 0x8000f)
	{
		// 모노 PCM WAV 파일을 재생함.
		hr = PlayPCM(pXAudio2, L".\\mono.wav");
		if (FAILED(hr)) return false;
	}
	if (GetAsyncKeyState('2') & 0x8000f)
	{
		// 5.1 PCM WAV 파일을 재생함.
		hr = PlayPCM(pXAudio2, L".\\surround.wav");
		if (FAILED(hr)) return false;
	}

	if (GetAsyncKeyState('Q') & 0x8000f)
	{
		//'q'가 아닌 'Q', 항상 대문자를 사용해야 함
		return false; //응용 프로그램을 종료함
	}

	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// 상태 정보 그리기

	WCHAR strbuf[180];
	wsprintf(strbuf, L"1: play mono WAV PCM file\n  2: play 5.1 WAV PCM file\n  ESC to cancel play\n  q to quit");

	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
	pRenderTarget->DrawText(strbuf, (int)wcslen(strbuf), pTextFormat, D2D1::RectF(0, 0, rtSize.width, rtSize.height), pBlackBrush);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
