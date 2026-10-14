#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <dwrite.h>
#pragma comment(lib, "dwrite")

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

class MyApp
{
public:
	MyApp() {};
	~MyApp() { DiscardDeviceResource(); DiscardAppResource(); };
	bool Initialize(HINSTANCE hInstance);
	void Render();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnResize();
	bool CreateAppResource();
	void DiscardAppResource();
	bool CreateDeviceResource();
	void DiscardDeviceResource();

	// 예제를 위한 함수 선언
	void RenderCombinedGeometry();
	bool UseGeometryFunction();

private:
	HWND hWnd = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;

	ID2D1RectangleGeometry* pRectangleGeometry = NULL;
	ID2D1EllipseGeometry* pEllipseGeometry = NULL;

	ID2D1SolidColorBrush* pBlackBrush = NULL;
	ID2D1SolidColorBrush* pYellowGreenBrush = NULL;

	ID2D1LinearGradientBrush* pLinearGradientBrush = NULL;
	ID2D1RadialGradientBrush* pRadialGradientBrush = NULL;

	// 획
	ID2D1StrokeStyle* pStrokeStyle = NULL;
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
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	if (FAILED(hr)) return false;

	// 사각형
	pD2DFactory->CreateRectangleGeometry(D2D1::RectF(20.f, 50.f, 180.f, 150.f), &pRectangleGeometry);

	// 타원
	pD2DFactory->CreateEllipseGeometry(D2D1::Ellipse(D2D1::Point2F(100.f, 100.f), 90.f, 50.f), &pEllipseGeometry);

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pStrokeStyle);
}

bool MyApp::CreateDeviceResource()
{
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

	// 검은 브러시 생성.
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &pBlackBrush);

	// 연두색 브러시 생성.
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x9ACD32, 1.0f), &pYellowGreenBrush);

	// 그라디언트 스톱 구조체인 D2D1_GRADIENT_STOP 구조체의 배열을 생성.
	D2D1_GRADIENT_STOP gradientStops[2];
	gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::Yellow, 1);
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::ForestGreen, 1);
	gradientStops[1].position = 1.0f;

	// D2D1_GRADIENT_STOP 구조체의 배열로부터 ID2D1GradientStopCollection 객체 생성.
	ID2D1GradientStopCollection* pGradientStops = NULL;
	pRenderTarget->CreateGradientStopCollection(gradientStops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);

	// ID2D1GradientStopCollection 객체로부터 선형 그라디언트 브러시 생성. 그라디언트 축은 사각형의 왼쪽 상단 모서리에서 시작해서 오른쪽 하단 모서리에서 끝남.
	pRenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(150, 150)), pGradientStops, &pLinearGradientBrush);

	// ID2D1GradientStopCollection 객체로부터 방사형 그라디언트 브러시 생성. 그라디언트의 중심은 상자의 중심이며 그라디언트 원점의 오프셋은 (0,0)임.
	pRenderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(100, 100), D2D1::Point2F(0, 0), 90, 50), pGradientStops, &pRadialGradientBrush);

	SAFE_RELEASE(pGradientStops);

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
}

// 여러 결합 옵션으로 두 기하를 결합하여 결합된 기하를 생성하고 이를 보여줌.
void MyApp::RenderCombinedGeometry()
{
	
}

// 기하 인터페이스의 함수들을 사용하는 예제를 보여줌.
bool MyApp::UseGeometryFunction()
{
	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// 단색 브러시
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(0, 0)));
	pRenderTarget->FillGeometry(pRectangleGeometry, pYellowGreenBrush);

	// 선형 그라디언트 브러시
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(200, 0)));
	pRenderTarget->FillGeometry(pRectangleGeometry, pLinearGradientBrush);

	// 방사형 그라디언트 브러시
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(400, 0)));
	pRenderTarget->FillGeometry(pRectangleGeometry, pRadialGradientBrush);

	// 단색 브러시
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(0, 200)));
	pRenderTarget->FillGeometry(pEllipseGeometry, pYellowGreenBrush);

	// 선형 그라디언트 브러시
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(200, 200)));
	pRenderTarget->FillGeometry(pEllipseGeometry, pLinearGradientBrush);

	// 방사형 그라디언트 브러시
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(400, 200)));
	pRenderTarget->FillGeometry(pEllipseGeometry, pRadialGradientBrush);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
