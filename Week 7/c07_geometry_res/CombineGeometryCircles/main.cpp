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

	// 예제를 위한 변수들 선언
	// 기하
	ID2D1EllipseGeometry* pCircleGeometry1 = NULL;
	ID2D1EllipseGeometry* pCircleGeometry2 = NULL;
	ID2D1PathGeometry* pPathGeometryUnion = NULL;
	ID2D1PathGeometry* pPathGeometryIntersect = NULL;
	ID2D1PathGeometry* pPathGeometryXOR = NULL;
	ID2D1PathGeometry* pPathGeometryExclude = NULL;
	// 붓
	ID2D1SolidColorBrush* pOutlineBrush = NULL;
	ID2D1SolidColorBrush* pTextBrush = NULL;
	ID2D1SolidColorBrush* pShapeFillBrush = NULL;
	// 획
	ID2D1StrokeStyle* pStrokeStyle = NULL;
	// DirectWrite
	IDWriteFactory* pDWriteFactory = NULL;
	IDWriteTextFormat* pTextFormat = NULL;
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

	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
	pDWriteFactory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"", &pTextFormat);

	float dashes[] = { 1.f, 1.f, 2.f, 3.f, 5.f };
	pD2DFactory->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND, 10.f, D2D1_DASH_STYLE_CUSTOM, 0.f), dashes, ARRAYSIZE(dashes) - 1, &pStrokeStyle);

	// 이제부터 기하를 생성함.

	//  첫 번째 기하를 생성함.
	const D2D1_ELLIPSE circle1 = D2D1::Ellipse(D2D1::Point2F(75.0f, 75.0f), 50.0f, 50.0f);
	pD2DFactory->CreateEllipseGeometry(circle1, &pCircleGeometry1);
	// 두 번째 기하를 생성함.
	const D2D1_ELLIPSE circle2 = D2D1::Ellipse(D2D1::Point2F(125.0f, 75.0f), 50.0f, 50.0f);
	pD2DFactory->CreateEllipseGeometry(circle2, &pCircleGeometry2);

	// 두 기하를 결합함. 결합 옵션으로 D2D1_COMBINE_MODE_UNION을 사용함.
	pD2DFactory->CreatePathGeometry(&pPathGeometryUnion);
	ID2D1GeometrySink* pGeometrySink = NULL;
	pPathGeometryUnion->Open(&pGeometrySink);
	pCircleGeometry1->CombineWithGeometry(pCircleGeometry2, D2D1_COMBINE_MODE_UNION, NULL, NULL, pGeometrySink);
	pGeometrySink->Close();
	SAFE_RELEASE(pGeometrySink);

	// 두 기하를 결합함. 결합 옵션으로 D2D1_COMBINE_MODE_INTERSECT을 사용함.
	pD2DFactory->CreatePathGeometry(&pPathGeometryIntersect);
	pPathGeometryIntersect->Open(&pGeometrySink);
	pCircleGeometry1->CombineWithGeometry(pCircleGeometry2, D2D1_COMBINE_MODE_INTERSECT, NULL, NULL, pGeometrySink);
	pGeometrySink->Close();
	SAFE_RELEASE(pGeometrySink);

	// 두 기하를 결합함. 결합 옵션으로 D2D1_COMBINE_MODE_XOR을 사용함.
	pD2DFactory->CreatePathGeometry(&pPathGeometryXOR);
	pPathGeometryXOR->Open(&pGeometrySink);
	pCircleGeometry1->CombineWithGeometry(pCircleGeometry2, D2D1_COMBINE_MODE_XOR, NULL, NULL, pGeometrySink);
	pGeometrySink->Close();
	SAFE_RELEASE(pGeometrySink);

	// 두 기하를 결합함. 결합 옵션으로 D2D1_COMBINE_MODE_EXCLUDE을 사용함.
	pD2DFactory->CreatePathGeometry(&pPathGeometryExclude);
	pPathGeometryExclude->Open(&pGeometrySink);
	pCircleGeometry1->CombineWithGeometry(pCircleGeometry2, D2D1_COMBINE_MODE_EXCLUDE, NULL, NULL, pGeometrySink);
	pGeometrySink->Close();
	SAFE_RELEASE(pGeometrySink);

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pCircleGeometry1);
	SAFE_RELEASE(pCircleGeometry2);
	SAFE_RELEASE(pPathGeometryUnion);
	SAFE_RELEASE(pPathGeometryIntersect);
	SAFE_RELEASE(pPathGeometryXOR);
	SAFE_RELEASE(pPathGeometryExclude);
	SAFE_RELEASE(pDWriteFactory);
	SAFE_RELEASE(pTextFormat);
	SAFE_RELEASE(pStrokeStyle);
}

bool MyApp::CreateDeviceResource()
{
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkSlateBlue, 1.f), &pOutlineBrush);
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue, 0.5f), &pShapeFillBrush);
	pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.f), &pTextBrush);

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pOutlineBrush);
	SAFE_RELEASE(pShapeFillBrush);
	SAFE_RELEASE(pTextBrush);
}

// 여러 결합 옵션으로 두 기하를 결합하여 결합된 기하를 생성하고 이를 보여줌.
void MyApp::RenderCombinedGeometry()
{
	// 결합 전의 기하들을 그림.
	pRenderTarget->FillGeometry(pCircleGeometry1, pShapeFillBrush);
	pRenderTarget->DrawGeometry(pCircleGeometry1, pOutlineBrush, 1.0f);
	pRenderTarget->FillGeometry(pCircleGeometry2, pShapeFillBrush);
	pRenderTarget->DrawGeometry(pCircleGeometry2, pOutlineBrush, 1.0f);
	static const WCHAR szBeforeText[] = L"The circles before combining";
	pRenderTarget->DrawText(szBeforeText, ARRAYSIZE(szBeforeText) - 1, pTextFormat, D2D1::RectF(25.0f, 130.0f, 200.0f, 300.0f), pTextBrush);

	// UNION 결합 모드로 결환된 기하를 그림.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(200, 0));
	pRenderTarget->FillGeometry(pPathGeometryUnion, pShapeFillBrush, NULL);
	pRenderTarget->DrawGeometry(pPathGeometryUnion, pOutlineBrush, 1.0f);
	static const WCHAR szUnionText[] = L"D2D1_COMBINE_MODE_UNION";
	pRenderTarget->DrawText(szUnionText, ARRAYSIZE(szUnionText) - 1, pTextFormat, D2D1::RectF(25, 130, 200, 300), pTextBrush);

	// INTERSECT 결합 모드로 결환된 기하를 그림.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(400, 0));
	pRenderTarget->FillGeometry(pPathGeometryIntersect, pShapeFillBrush, NULL);
	pRenderTarget->DrawGeometry(pPathGeometryIntersect, pOutlineBrush, 1.0f);
	static const WCHAR szIntersectText[] = L"D2D1_COMBINE_MODE_INTERSECT";
	pRenderTarget->DrawText(szIntersectText, ARRAYSIZE(szIntersectText) - 1, pTextFormat, D2D1::RectF(25, 130, 400, 300), pTextBrush);

	// XOR 결합 모드로 결환된 기하를 그림.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(200, 150));
	pRenderTarget->FillGeometry(pPathGeometryXOR, pShapeFillBrush, NULL);
	pRenderTarget->DrawGeometry(pPathGeometryXOR, pOutlineBrush, 1.0f);
	static const WCHAR szXorText[] = L"D2D1_COMBINE_MODE_XOR";
	pRenderTarget->DrawText(szXorText, ARRAYSIZE(szXorText) - 1, pTextFormat, D2D1::RectF(25, 130, 200, 470), pTextBrush);

	// EXCLUDE 결합 모드로 결환된 기하를 그림.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(400, 150));
	pRenderTarget->FillGeometry(pPathGeometryExclude, pShapeFillBrush, NULL);
	pRenderTarget->DrawGeometry(pPathGeometryExclude, pOutlineBrush, 1.0f);
	static const WCHAR szExcludeText[] = L"D2D1_COMBINE_MODE_EXCLUDE";
	pRenderTarget->DrawText(szExcludeText, ARRAYSIZE(szExcludeText) - 1, pTextFormat, D2D1::RectF(25, 130, 400, 470), pTextBrush);
}

// 기하 인터페이스의 함수들을 사용하는 예제를 보여줌.
bool MyApp::UseGeometryFunction()
{
	pRenderTarget->SetTransform(D2D1::IdentityMatrix());

	// 함수 CompareWithGeometry의 사용 예시.
	D2D1_GEOMETRY_RELATION result = D2D1_GEOMETRY_RELATION_UNKNOWN;
	// 두 원을 비교함.
	pCircleGeometry1->CompareWithGeometry(pCircleGeometry2, D2D1::IdentityMatrix(), 0.1f, &result);
	if (result == D2D1_GEOMETRY_RELATION_OVERLAP)
	{
		static const WCHAR szGeometryRelation[] = L"Two circles overlap.";
		pRenderTarget->DrawText(szGeometryRelation, ARRAYSIZE(szGeometryRelation) - 1, pTextFormat, D2D1::RectF(25.0f, 160.0f, 200.0f, 300.0f), pTextBrush);
	}

	// 함수 ComputeArea의 사용 예시.
	float area;
	pCircleGeometry1->ComputeArea(D2D1::IdentityMatrix(), &area);
	// 여기서 넓이 area를 활용하면 됨...

	// 함수 ComputeLength의 사용 예시.
	float length;
	pCircleGeometry1->ComputeLength(D2D1::IdentityMatrix(), &length);
	// 여기서 기하 길이 length를 활용하면 됨...

	// 함수 ComputePointAtLength의 사용 예시.
	D2D1_POINT_2F point;
	D2D1_POINT_2F tangent;
	// 길이 10에 해당하는 기하 상의 지점을 요청함.
	pCircleGeometry1->ComputePointAtLength(10, NULL, &point, &tangent);
	// 여기서 위치 point와 접선벡터 tangent를 활용하면 됨...

	// 함수 GetBounds의 사용 예시.
	D2D1_RECT_F bounds;
	pCircleGeometry1->GetBounds(D2D1::IdentityMatrix(), &bounds);
	// 여기서 bounds를 활용하면 됨...

	// 함수 GetWidenedBounds의 사용 예시.
	D2D1_RECT_F bounds1;
	pCircleGeometry1->GetWidenedBounds(5.0, pStrokeStyle, D2D1::IdentityMatrix(), &bounds1);
	// 여기서 widened bounds를 활용하면 됨...

	// 함수 StrokeContainsPoint의 사용 예시.
	BOOL containsPoint;
	pCircleGeometry1->StrokeContainsPoint(D2D1::Point2F(0, 0), 10, NULL, NULL, &containsPoint);
	// 여기서 containsPoint를 활용하면 됨...

	// 함수 FillContainsPoint의 사용 예시.
	BOOL containsPoint1;
	pCircleGeometry1->FillContainsPoint(D2D1::Point2F(0, 0), D2D1::Matrix3x2F::Identity(), &containsPoint1);
	// 여기서 containsPoint를 활용하면 됨...

	return true;
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// 기하 결합을 보여줌.
	RenderCombinedGeometry();

	// 기하 함수들의 사용을 보여줌.
	UseGeometryFunction();

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}
