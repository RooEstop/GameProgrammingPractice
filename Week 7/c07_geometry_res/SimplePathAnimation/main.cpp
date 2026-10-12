#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <math.h>

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

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
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;

	// 예제를 위한 변수들 선언
	// 기하
	ID2D1PathGeometry* pPathGeometry = NULL;
	ID2D1PathGeometry* pObjectGeometry = NULL;
	// 붓
	ID2D1SolidColorBrush* pRedBrush = NULL;
	ID2D1SolidColorBrush* pYellowBrush = NULL;

	// 애니메이션 변수
	float timeDuration; // 애니매이션 지속 시간. 분모로 사용되므로 0이 되면 안됨.
	float alpha; // 애니메이션 동안의 현재 위치값, [0,1] 사이의 값.
	float startPos;
	float endPos;
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
			LARGE_INTEGER CurrentTime;
			QueryPerformanceCounter(&CurrentTime);
			float timeDelta = (float)((double)(CurrentTime.QuadPart - nPrevTime.QuadPart) / (double)(nFrequency.QuadPart));
			nPrevTime = CurrentTime;

			// 갱신 함수를 호출한다. false가 리턴되면 종료한다.
			if (!myApp.Update(timeDelta)) break;

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

	// 나선형 모양의 경로 기하를 생성함.

	pD2DFactory->CreatePathGeometry(&pPathGeometry);
	ID2D1GeometrySink* pSink = NULL;
	pPathGeometry->Open(&pSink);

	D2D1_POINT_2F currentLocation = { 0, 0 };

	pSink->BeginFigure(currentLocation, D2D1_FIGURE_BEGIN_FILLED);

	D2D1_POINT_2F locDelta = { 2, 2 };
	float radius = 3;

	for (UINT i = 0; i < 30; ++i)
	{
		currentLocation.x += radius * locDelta.x;
		currentLocation.y += radius * locDelta.y;

		pSink->AddArc(
			D2D1::ArcSegment(
				currentLocation,
				D2D1::SizeF(2 * radius, 2 * radius), // radiusx/y
				0.0f, // rotation angle
				D2D1_SWEEP_DIRECTION_CLOCKWISE,
				D2D1_ARC_SIZE_SMALL
			)
		);

		locDelta = D2D1::Point2F(-locDelta.y, locDelta.x);

		radius += 3;
	}

	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	pSink->Close();
	SAFE_RELEASE(pSink);

	// 간단한 삼각형 모양의 경로 기하를 생성함.

	pD2DFactory->CreatePathGeometry(&pObjectGeometry);
	pObjectGeometry->Open(&pSink);
	pSink->BeginFigure(D2D1::Point2F(0.0f, 0.0f), D2D1_FIGURE_BEGIN_FILLED);

	const D2D1_POINT_2F ptTriangle[] = { {-10.0f, -10.0f}, {-10.0f, 10.0f}, {0.0f, 0.0f} };
	pSink->AddLines(ptTriangle, 3);

	pSink->EndFigure(D2D1_FIGURE_END_OPEN);
	pSink->Close();
	SAFE_RELEASE(pSink);

	// 애니메이션 초기화
	timeDuration = 5.0f;
	alpha = 0;
	startPos = 0;
	// endPos 결정
	float length = 0;
	pPathGeometry->ComputeLength(NULL, &length);
	endPos = length;

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pPathGeometry);
	SAFE_RELEASE(pObjectGeometry);
}

bool MyApp::CreateDeviceResource()
{
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pRedBrush);

	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow), &pYellowBrush);

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pRedBrush);
	SAFE_RELEASE(pYellowBrush);
}

float GetPosInterp(float t, float s, float e)
{
	float u;

	// linear
	//u = t;

	// easein cubic
	//u = t * t * t;

	// easeout cubic
	//u = (float)(1 - pow(1 - t, 3));

	// easeinout cubic
	u = (t < 0.5) ? (4 * t * t * t) : (float)(1 - pow(-2 * t + 2, 3) / 2);

	return (s + ((e - s) * u));
}

bool MyApp::Update(float timeDelta)
{
	static float timeElapsed = 0;
	timeElapsed += timeDelta;

	if (timeElapsed >= timeDuration) timeElapsed = 0.0f;
	alpha = timeElapsed / timeDuration;

	// 응용 프로그램을 종료하는 경우에 false를 리턴하고 그 외에는 항상 true를 리턴함.
	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw(); // 그리기를 준비함.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity()); // 변환을 항등행렬로 리셋함.
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black)); // 렌더타겟을 클리어함.

	// 이동 동선 그리기

	// 이동 동선 기하 경로가 화면 중심에 그려지도록 함.
	D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
	float minWidthHeightScale = min(rtSize.width, rtSize.height) / 512;
	D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(minWidthHeightScale, minWidthHeightScale);
	D2D1::Matrix3x2F translation = D2D1::Matrix3x2F::Translation(rtSize.width / 2, rtSize.height / 2);
	pRenderTarget->SetTransform(scale * translation);

	pRenderTarget->DrawGeometry(pPathGeometry, pRedBrush); // 이동 동선을 붉은색으로 그림.

	// 삼각형 그리기

	// 현재 시간에 해당하는 기하 길이에 일치하는 이동 동선 상의 지점을 얻음.
	float length = GetPosInterp(alpha, startPos, endPos); // 애니메이션을 위한 현재 위치 계산
	D2D1_POINT_2F point;
	D2D1_POINT_2F tangent;
	pPathGeometry->ComputePointAtLength(length, NULL, &point, &tangent);

	// 삼각형의 방향을 조절하여 이동 동선을 따라가는 방향이 되도록 함.
	D2D1_MATRIX_3X2_F triangleMatrix;
	triangleMatrix = D2D1::Matrix3x2F(
		tangent.x, tangent.y,
		-tangent.y, tangent.x,
		point.x, point.y);
	pRenderTarget->SetTransform(triangleMatrix * scale * translation);

	pRenderTarget->FillGeometry(pObjectGeometry, pYellowBrush); // 삼각형을 노란색으로 그림.

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
