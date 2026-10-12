#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <dwrite.h>
#pragma comment(lib, "dwrite")

// 자원 안전 반환 매크로.
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

class MyApp
{
public:
	MyApp() {};
	~MyApp() { DiscardDeviceResource(); DiscardAppResource(); }; // 소멸자. 모든 자원을 반납함.
	bool Initialize(HINSTANCE hInstance); // 윈도우 생성, CreateAppResource 호출 포함.
	void Render(); // 내용을 그리기, CreateDevice 호출 포함.

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //윈도우 프로시져
	void OnResize(); //창 리사이즈 콜백 함수
	bool CreateAppResource(); //앱 자원 생성
	void DiscardAppResource(); //앱 자원 폐기
	bool CreateDeviceResource(); //장치 자원 생성
	void DiscardDeviceResource(); //장치 자원 폐기

	// 예제를 위해 추가된 함수
	void DrawAndTransformRectangle(float x, float y, float width, float height, float bx, float by, D2D1::Matrix3x2F mat, const WCHAR* str);

private:
	//클래스의 변수들을 선언
	HWND hWnd = NULL;
	ID2D1Factory* pD2DFactory = NULL;
	ID2D1HwndRenderTarget* pRenderTarget = NULL;
	ID2D1StrokeStyle* pStrokeStyleDash = NULL;
	ID2D1SolidColorBrush* pOriginalShapeBrush = NULL;
	ID2D1SolidColorBrush* pTransformedShapeBrush = NULL;
	ID2D1SolidColorBrush* pFillBrush = NULL;
	ID2D1SolidColorBrush* pTextBrush = NULL;
	//DWrite
	IDWriteFactory* pDWriteFactory = NULL;
	IDWriteTextFormat* pTextFormat = NULL;
};

// 응용 프로그램의 진입점 함수.
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

// 응용 프로그램의 원도우를 생성하고, 장치 독립적 자원을 생성함.
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
	hWnd = CreateWindow(L"MyApp", L"MyApp", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 250, NULL, NULL, hInstance, this);
	if (!hWnd) return false;

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// 장치 독립적 자원을 생성함.
	if (!CreateAppResource()) return false;
	return true;
}

// 윈도우 메시지를 처리.
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

// 앱 자원들을 생성함. 이들 자원의 수명은 응용 프로그램이 종료되기 전까지 유효함.
bool MyApp::CreateAppResource()
{
	// D2D 팩토리를 생성함.
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
	if (FAILED(hr)) return false;

	// DirectWrite 팩토리를 생성함.
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
	if (FAILED(hr)) return false;

	//DirectWrite 텍스트 포맷 객체를 생성함.
	hr = pDWriteFactory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"", &pTextFormat);
	if (FAILED(hr)) return false;

	// 점선 획 스타일 객체를 생성함.
	float dashes[] = { 10.0f, 2.0f };
	hr = pD2DFactory->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f), dashes, ARRAYSIZE(dashes), &pStrokeStyleDash);
	if (FAILED(hr)) return false;

	return true;
}

// 앱 자원을 폐기함.
void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pStrokeStyleDash);
	SAFE_RELEASE(pDWriteFactory);
	SAFE_RELEASE(pTextFormat);
}

// 장치 자원을 생성함. 장치가 소실되는 경우에는 이들 자원을 다시 생성해야 함.
bool MyApp::CreateDeviceResource()
{
	// 렌더타겟이 유효하다면 이 함수를 실행할 필요가 없음
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

	// D2D 렌더타겟을 생성함.
	HRESULT hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);
	if (FAILED(hr)) return false;

	// 변환 전 모양의 외곽선을 위한 붓을 생성.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkBlue), &pOriginalShapeBrush);
	if (FAILED(hr)) return false;
	pOriginalShapeBrush->SetOpacity(0.5f); // 브러시를 반투명하게 명시함

	// 변환 후 영역을 채우는 용도의 붓을 생성.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pFillBrush);
	if (FAILED(hr)) return false;
	pFillBrush->SetOpacity(0.5f); // 브러시를 반투명하게 명시함

	// 텍스트를 쓰기 위한 붓을 생성.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pTextBrush);
	if (FAILED(hr)) return false;

	// 변환 후 모양의 외곽선을 위한 붓을 생성.
	hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkBlue), &pTransformedShapeBrush);
	if (FAILED(hr)) return false;

	return true;
}

// 장치 자원을 폐기함. 장치가 소실되면 장치 자원을 폐기한 후에 다시 생성해야 함.
void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pTransformedShapeBrush);
	SAFE_RELEASE(pFillBrush);
	SAFE_RELEASE(pOriginalShapeBrush);
	SAFE_RELEASE(pTextBrush);
}

// 그릴 내용을 그리기.
void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	// 그리기를 시작함.
	pRenderTarget->BeginDraw();

	// 렌더타겟 변환을 항등 변환으로 리셋함.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

	// 렌더타겟 내용을 클리어함.
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	const float x = 30.f;
	const float y = 50.f;
	const float width = 60.f;
	const float height = 60.f;
	float bx = 0;
	float by = 0;

	// 이동 변환
	DrawAndTransformRectangle(x, y, width, height, bx, by, D2D1::Matrix3x2F::Translation(20, 10), L"Translate");
	// 회전 변환
	bx = 150;
	DrawAndTransformRectangle(x, y, width, height, bx, by, D2D1::Matrix3x2F::Rotation(45.0f, D2D1::Point2F(x + (width / 2) + bx, y + (height / 2) + by)), L"Rotate");
	// 크기조정 변환
	bx = 300;
	DrawAndTransformRectangle(x, y, width, height, bx, by, D2D1::Matrix3x2F::Scale(D2D1::Size(1.3f, 1.3f), D2D1::Point2F(x + bx, y + by)), L"Scale");
	// 기울임 변환
	bx = 450;
	DrawAndTransformRectangle(x, y, width, height, bx, by, D2D1::Matrix3x2F::Skew(45.0f, 0.0f, D2D1::Point2F(x + bx, y + by)), L"Skew");

	// 그리기 연산들을 제출함.
	HRESULT hr = pRenderTarget->EndDraw();
	// 이 함수는 실행되는 동안에 장치가 소실되면 장치 자원들을 반납함.
	// 다음번 Render 호출 시에 내부에서 호출되는 CreateDeviceResource 에서 반납된 장치 자원들이 다시 생성됨.
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}

// 변환을 렌더타겟에 지정하고 그림.
void MyApp::DrawAndTransformRectangle(float x, float y, float width, float height, float bx, float by, D2D1::Matrix3x2F mat, const WCHAR* str)
{
	D2D1_RECT_F rectangle = D2D1::Rect(x + bx, y + by, x + width + bx, y + height + by);

	// 렌더타겟 변환을 항등 변환으로 리셋함. 사각형을 그림.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->DrawRectangle(rectangle, pOriginalShapeBrush, 1.0f, pStrokeStyleDash);

	// 이동 변환을 렌더타겟에 지정하고 사각형을 채워 그림.
	pRenderTarget->SetTransform(mat);
	pRenderTarget->FillRectangle(rectangle, pFillBrush);
	pRenderTarget->DrawRectangle(rectangle, pTransformedShapeBrush);

	// 캡션 텍스트를 표시함.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->DrawText(str, lstrlen(str), pTextFormat, D2D1::RectF(x + bx, y + height + by + 30, x + width + bx + 50, y + height + by + 50), pTextBrush);
}
