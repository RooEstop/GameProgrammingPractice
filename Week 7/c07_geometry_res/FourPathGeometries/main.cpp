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
	// 붓
	ID2D1SolidColorBrush* pSceneBrush = NULL;
	ID2D1RadialGradientBrush* pRadialGradientBrush = NULL;
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

	// 첫번째 경로기하 만들기
	pD2DFactory->CreatePathGeometry(&pLeftMountainGeometry);
	ID2D1GeometrySink* pSink = NULL;
	pLeftMountainGeometry->Open(&pSink);
	pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
	pSink->BeginFigure(D2D1::Point2F(346, 255), D2D1_FIGURE_BEGIN_FILLED);
	D2D1_POINT_2F points1[5] = { D2D1::Point2F(267, 177), D2D1::Point2F(236, 192), D2D1::Point2F(212, 160), D2D1::Point2F(156, 255), D2D1::Point2F(346, 255) };
	pSink->AddLines(points1, ARRAYSIZE(points1));
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
	pSink->Close();
	SAFE_RELEASE(pSink);

	// 두번째 경로기하 만들기
	pD2DFactory->CreatePathGeometry(&pRightMountainGeometry);
	pRightMountainGeometry->Open(&pSink);
	pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
	pSink->BeginFigure(D2D1::Point2F(575, 263), D2D1_FIGURE_BEGIN_FILLED);
	D2D1_POINT_2F points2[] = { D2D1::Point2F(481, 146), D2D1::Point2F(449, 181), D2D1::Point2F(433, 159), D2D1::Point2F(401, 214), D2D1::Point2F(381, 199), D2D1::Point2F(323, 263), D2D1::Point2F(575, 263) };
	pSink->AddLines(points2, ARRAYSIZE(points2));
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
	pSink->Close();
	SAFE_RELEASE(pSink);

	// 세번째 경로기하 만들기
	pD2DFactory->CreatePathGeometry(&pSunGeometry);
	pSunGeometry->Open(&pSink);
	pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
	//
	pSink->BeginFigure(D2D1::Point2F(270, 255), D2D1_FIGURE_BEGIN_FILLED);
	pSink->AddArc(D2D1::ArcSegment(D2D1::Point2F(440, 255), D2D1::SizeF(85, 85), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
	//
	pSink->BeginFigure(D2D1::Point2F(299, 182), D2D1_FIGURE_BEGIN_HOLLOW);
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(299, 182), D2D1::Point2F(294, 176), D2D1::Point2F(285, 178)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(276, 179), D2D1::Point2F(272, 173), D2D1::Point2F(272, 173)));
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	//
	pSink->BeginFigure(D2D1::Point2F(354, 156), D2D1_FIGURE_BEGIN_HOLLOW);
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(354, 156), D2D1::Point2F(358, 149), D2D1::Point2F(354, 142)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(349, 134), D2D1::Point2F(354, 127), D2D1::Point2F(354, 127)));
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	//
	pSink->BeginFigure(D2D1::Point2F(322, 164), D2D1_FIGURE_BEGIN_HOLLOW);
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(322, 164), D2D1::Point2F(322, 156), D2D1::Point2F(314, 152)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(306, 149), D2D1::Point2F(305, 141), D2D1::Point2F(305, 141)));
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	//
	pSink->BeginFigure(D2D1::Point2F(385, 164), D2D1_FIGURE_BEGIN_HOLLOW);
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(385, 164), D2D1::Point2F(392, 161), D2D1::Point2F(394, 152)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(395, 144), D2D1::Point2F(402, 141), D2D1::Point2F(402, 142)));
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	//
	pSink->BeginFigure(D2D1::Point2F(408, 182), D2D1_FIGURE_BEGIN_HOLLOW);
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(408, 182), D2D1::Point2F(416, 184), D2D1::Point2F(422, 178)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(428, 171), D2D1::Point2F(435, 173), D2D1::Point2F(435, 173)));
	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	pSink->Close();
	SAFE_RELEASE(pSink);

	// 네번째 경로기하 만들기
	pD2DFactory->CreatePathGeometry(&pRiverGeometry);
	pRiverGeometry->Open(&pSink);
	pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
	pSink->BeginFigure(D2D1::Point2F(183, 392), D2D1_FIGURE_BEGIN_FILLED);
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(238, 284), D2D1::Point2F(472, 345), D2D1::Point2F(356, 303)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(237, 261), D2D1::Point2F(333, 256), D2D1::Point2F(333, 256)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(335, 257), D2D1::Point2F(241, 261), D2D1::Point2F(411, 306)));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(574, 350), D2D1::Point2F(288, 324), D2D1::Point2F(296, 392)));
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

	pRenderTarget->FillGeometry(pSunGeometry, pRadialGradientBrush);

	pSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 1.f));
	pRenderTarget->DrawGeometry(pSunGeometry, pSceneBrush, 1.f);

	pSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::OliveDrab, 1.f));
	pRenderTarget->FillGeometry(pLeftMountainGeometry, pSceneBrush);

	pSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 1.f));
	pRenderTarget->DrawGeometry(pLeftMountainGeometry, pSceneBrush, 1.f);

	pSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::LightSkyBlue, 1.f));
	pRenderTarget->FillGeometry(pRiverGeometry, pSceneBrush);

	pSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 1.f));
	pRenderTarget->DrawGeometry(pRiverGeometry, pSceneBrush, 1.f);

	pSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::YellowGreen, 1.f));
	pRenderTarget->FillGeometry(pRightMountainGeometry, pSceneBrush);

	pSceneBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 1.f));
	pRenderTarget->DrawGeometry(pRightMountainGeometry, pSceneBrush, 1.f);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
