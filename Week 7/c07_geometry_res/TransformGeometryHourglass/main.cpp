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
	ID2D1PathGeometry* pPathGeometry = NULL;
	ID2D1TransformedGeometry* pTransformedGeometry = NULL;
	// 붓
	ID2D1LinearGradientBrush* pLGBrush = NULL;
	ID2D1SolidColorBrush* pBlackBrush = NULL;
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

	// 경로 기하를 생성함.
	pD2DFactory->CreatePathGeometry(&pPathGeometry);
	// 기하 싱크를 이용하여 경로 기하에 쓰기 시작
	ID2D1GeometrySink* pSink = NULL;
	pPathGeometry->Open(&pSink);
	pSink->BeginFigure(D2D1::Point2F(0, 0), D2D1_FIGURE_BEGIN_FILLED);
	pSink->AddLine(D2D1::Point2F(200, 0));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(150, 50), D2D1::Point2F(150, 150), D2D1::Point2F(200, 200)));
	pSink->AddLine(D2D1::Point2F(0, 200));
	pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(50, 150), D2D1::Point2F(50, 50), D2D1::Point2F(0, 0)));
	pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
	pSink->Close();
	SAFE_RELEASE(pSink);

	// 위의 경로 기하를 45도 시계방향으로 회전시켜 변환된 기하를 생성함.
	pD2DFactory->CreateTransformedGeometry(pPathGeometry,
		D2D1::Matrix3x2F::Rotation(45.f, D2D1::Point2F(100, 100)),
		&pTransformedGeometry);

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pPathGeometry);
	SAFE_RELEASE(pTransformedGeometry);
}

bool MyApp::CreateDeviceResource()
{
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

	// 검정색 단색 붓
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush);

	// 선형 계조 붓
	static const D2D1_GRADIENT_STOP stops[] =
	{
		{   0.f,  { 0.f, 1.f, 1.f, 0.25f }  },
		{   1.f,  { 0.f, 0.f, 1.f, 1.f }  },
	};
	ID2D1GradientStopCollection* pGradientStops = NULL;
	pRenderTarget->CreateGradientStopCollection(stops, ARRAYSIZE(stops), &pGradientStops);
	pRenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(100, 0), D2D1::Point2F(100, 200)), D2D1::BrushProperties(), pGradientStops, &pLGBrush);
	SAFE_RELEASE(pGradientStops);

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pLGBrush);
	SAFE_RELEASE(pBlackBrush);
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// 클라이언트 영역 계산하기
	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

	// 원본 경로 기하를 왼쪽 위에 그림.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(20.f, 20.f));
	pRenderTarget->DrawGeometry(pPathGeometry, pBlackBrush, 10.f);
	pRenderTarget->FillGeometry(pPathGeometry, pLGBrush);

	// 이후의 그리기는 절반 크기로 크기조정된 후에 화면 중으로 이동하는 변환을 적용받음.
	pRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(D2D1::SizeF(0.5f, 0.5f), D2D1::Point2F(0.f, 0.f))
		*
		D2D1::Matrix3x2F::Translation(size.width / 2 - 50.f, size.height / 2 - 50.f)
	);

	// 원본 경로 기하를 그림.
	pRenderTarget->FillGeometry(pPathGeometry, pLGBrush);
	// 변환된 경로 기하를 그림.
	pRenderTarget->FillGeometry(pTransformedGeometry, pLGBrush);

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
