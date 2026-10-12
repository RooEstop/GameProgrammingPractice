#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")

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

private:
	HWND hWnd = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;

	// 예제를 위한 변수들 선언
	// 기하
	ID2D1PathGeometry* pSunGeometry = NULL;
	ID2D1PathGeometry* pLeftMountainGeometry = NULL;
	ID2D1PathGeometry* pRightMountainGeometry = NULL;
	ID2D1PathGeometry* pRiverGeometry = NULL;
	ID2D1PathGeometry* pPathGeometry1 = NULL;
	ID2D1PathGeometry* pPathGeometry2 = NULL;

	// 붓
	ID2D1SolidColorBrush* pSceneBrush = NULL;
	ID2D1RadialGradientBrush* pRadialGradientBrush = NULL;

	ID2D1SolidColorBrush* pBlackBrush = NULL;
	ID2D1SolidColorBrush* pCornflowerBlueBrush = NULL;
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
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);

	// 첫 번째 패스 지오메트리 생성; 선분 조각들로 구성된 비직각 사각형의 모습의 패스 지오메트리를 생성함
	pD2DFactory->CreatePathGeometry(&pPathGeometry1);
	ID2D1GeometrySink* pSink = NULL;
	pPathGeometry1->Open(&pSink);
	pSink->BeginFigure(D2D1::Point2F(0, 0), D2D1_FIGURE_BEGIN_FILLED);
	D2D1_POINT_2F points[3] = { D2D1::Point2F(150, 0), D2D1::Point2F(250, 100), D2D1::Point2F(100, 200) };
	pSink->AddLines(points, 3);
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
	pSink->Close();
	SAFE_RELEASE(pSink);

	// 두 번째 패스 지오메트리 생성; 여러 타입의 조각들이 혼합된 태극 모양의 패스 지오메트리를 생성함
	pD2DFactory->CreatePathGeometry(&pPathGeometry2);
	//ID2D1GeometrySink* pSink = NULL;
	pPathGeometry2->Open(&pSink);
	pSink->BeginFigure(D2D1::Point2F(0, 100), D2D1_FIGURE_BEGIN_FILLED);
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(100, 200), D2D1::Point2F(200, 0), D2D1::Point2F(300, 100)));
	pSink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0, 100), D2D1::SizeF(100, 100), 0.f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_LARGE));
	pSink->AddArc(D2D1::ArcSegment(D2D1::Point2F(300, 100), D2D1::SizeF(100, 100), 0.f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_LARGE));
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	pSink->Close();
	SAFE_RELEASE(pSink);

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pLeftMountainGeometry);
	SAFE_RELEASE(pRightMountainGeometry);
	SAFE_RELEASE(pSunGeometry);
	SAFE_RELEASE(pRiverGeometry);
}

bool MyApp::CreateDeviceResource()
{
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

	// 붓 생성
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.f), &pSceneBrush);

	HRESULT hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
	if (FAILED(hr)) return false;

	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush);
	if (FAILED(hr)) return false;

	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue), &pCornflowerBlueBrush);
	if (FAILED(hr)) return false;

	// 방사형 계조 붓을 생성함.
	// D2D1_GRADIENT_STOP 구조체 배열을 생성함.
	ID2D1GradientStopCollection* pGradientStops = NULL;

	D2D1_GRADIENT_STOP gradientStops[3];
	gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::Gold, 1);
	gradientStops[0].position = 0.0f;
	gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::Orange, 0.8f);
	gradientStops[1].position = 0.85f;
	gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::OrangeRed, 0.7f);
	gradientStops[2].position = 1.0f;

	// D2D1_GRADIENT_STOP 구조체 배열로부터 ID2D1GradientStopCollection을 생성함.
	pRenderTarget->CreateGradientStopCollection(gradientStops, 3, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops);

	// ID2D1GradientStopCollection로부터 방사형 계조 붓을 생성함.
	pRenderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(330, 330), D2D1::Point2F(140, 140), 140, 140), pGradientStops, &pRadialGradientBrush);

	SAFE_RELEASE(pGradientStops);

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pSceneBrush);
	SAFE_RELEASE(pRadialGradientBrush);
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// 첫 번째 패스 지오메트리 그리기
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(20, 100));
	pRenderTarget->FillGeometry(pPathGeometry1, pCornflowerBlueBrush);
	pRenderTarget->DrawGeometry(pPathGeometry1, pBlackBrush, 5.f, 0);

	// 두 번째 패스 지오메트리 그리기
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(300, 100));
	pRenderTarget->DrawGeometry(pPathGeometry2, pBlackBrush, 5.f, 0);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();

}
