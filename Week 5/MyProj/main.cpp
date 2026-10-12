#include <Windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

#define SAFE_RELEASE(p) {if(p) { (p)->Release(); (p) = NULL;} }

class MyApp
{
public:
	MyApp() {  };
	~MyApp() { DiscardDeviceResource(); DiscardAppResource(); };
	bool Initialize(HINSTANCE hInstance);
	void Render();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); // 윈도우 프로시져
	void OnResize(); // 창 리사이즈 콜백 함수
	bool CreateAppResource(); // 앱 자원 생성
	void DiscardAppResource(); // 앱 자원 폐기
	bool CreateDeviceResource(); // 장치 자원 생성
	void DiscardDeviceResource(); // 장치 자원 폐기

private:
	// 클래스의 변수들을 선언
	HWND hWnd = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;
	ID2D1SolidColorBrush* pLightSlateGrayBrush = NULL;
	ID2D1SolidColorBrush* pCornflowerBlueBrush = NULL;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MyApp myApp;
	if (!myApp.Initialize(hInstance)) return 1;

	// 메인 윈도우 메시지 루프를 실행함.
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

	// 윈도우를 생성함.
	hWnd = CreateWindow(L"MyApp", L"MyApp", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, this);
	if (!hWnd) return false;

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// 앱 자원을 생성함.
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

// 창 리사이즈 콜백 함수
void MyApp::OnResize()
{
	if (!pRenderTarget) return;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	// 렌더타겟의 크기를 다시 설정함.
	pRenderTarget->Resize(size);
}

bool MyApp::CreateAppResource()
{
	//D2D 팩토리를 생성함.
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	return SUCCEEDED(hr);
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
}

bool MyApp::CreateDeviceResource()
{
	//유효하다면 이 함수를 실행할 필요가 없음
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

	// D2D 렌더타겟을 생성함.
	HRESULT hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
	if (FAILED(hr)) return false;

	// 회색 브러시를 생성함.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &pLightSlateGrayBrush);
	if (FAILED(hr)) return false;

	// 파란색 브러시를 생성함.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue), &pCornflowerBlueBrush);
	if (FAILED(hr)) return false;

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pLightSlateGrayBrush);
	SAFE_RELEASE(pCornflowerBlueBrush);
}

// 그릴 내용을 그리기.
void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();

	// 격자 배경을 그림.

	for (int x = 0; x < (int)rtSize.width; x += 10)
	{
		pRenderTarget->DrawLine(
			D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
			D2D1::Point2F(static_cast<FLOAT>(x), rtSize.height),
			pLightSlateGrayBrush, 0.5f
		);
	}

	for (int y = 0; y < (int)rtSize.height; y += 10)
	{
		pRenderTarget->DrawLine(
			D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
			D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y)),
			pLightSlateGrayBrush, 0.5f
		);
	}

	// 두 개의 사각형을 그림.

	// 채워진 사각형을 그림.
	D2D1_RECT_F rectangle1 = D2D1::RectF(
		rtSize.width / 2 - 50.0f, rtSize.height / 2 - 50.0f,
		rtSize.width / 2 + 50.0f, rtSize.height / 2 + 50.0f
	);
	pRenderTarget->FillRectangle(&rectangle1, pLightSlateGrayBrush);

	// 채우지 않은 사각형을 그림.
	D2D1_RECT_F rectangle2 = D2D1::RectF(
		rtSize.width / 2 - 100.0f, rtSize.height / 2 - 100.0f,
		rtSize.width / 2 + 100.0f, rtSize.height / 2 + 100.0f
	);
	pRenderTarget->DrawRectangle(&rectangle2, pCornflowerBlueBrush);

	HRESULT hr = pRenderTarget->EndDraw();
	// 이 함수는 실행되는 동안에 장치가 소실되면 장치 자원들을 반납함.
	// 다음번 Render 호출 시에 내부에서 호출되는 CreateDeviceResource() 에서 반납된 장치 자원들이 다시 생성됨.
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}