#include <windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <wincodec.h>
#pragma comment(lib, "WindowsCodecs")

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// 비트맵을 로드하는 함수 원형
HRESULT LoadBitmapFromFile(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap);
HRESULT LoadBitmapFromResource(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap);

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
	// WIC
	IWICImagingFactory* pWICFactory = NULL;
	// 비트맵
	ID2D1Bitmap* pBitmapFromFile = NULL;
	ID2D1Bitmap* pBitmapFromResource = NULL;
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
	// CoCreateInstance 함수가 사용될 때에는 CoInitialize를 호출해주어야 함.
	CoInitialize(NULL); // CreateAppResource 함수의 처음에서 호출.

	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);

	// CoCreateInstance 함수를 사용하여 WIC 팩토리를 생성함.
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
	if (FAILED(hr)) return false;

	return true;
}

void MyApp::DiscardAppResource()
{
	SAFE_RELEASE(pD2DFactory);
	SAFE_RELEASE(pWICFactory);

	// CoInitialize 함수가 호출될 때에는 마지막에 이 함수도 호출해 주어야 함.
	CoUninitialize(); // DiscardAppResource 함수의 마지막에서 호출.
}

bool MyApp::CreateDeviceResource()
{
	if (pRenderTarget) return true;

	RECT rc;
	GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
	pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &pRenderTarget);

	HRESULT hr = S_OK;

	// 외부 파일로부터 비트맵 객체 pAnotherBitmap를 생성함.
	hr = LoadBitmapFromFile(pRenderTarget, pWICFactory, L".\\campus.jpg", 0, 0, &pBitmapFromFile);
	if (FAILED(hr)) return false;

	// 응용 프로그램 리소스로부터 비트맵 객체 pBitmap를 생성함.
	hr = LoadBitmapFromResource(pRenderTarget, pWICFactory, L"MyImage", L"Image", 0, 150, &pBitmapFromResource);
	if (FAILED(hr)) return false;

	return true;
}

void MyApp::DiscardDeviceResource()
{
	SAFE_RELEASE(pRenderTarget);
	SAFE_RELEASE(pBitmapFromFile);
	SAFE_RELEASE(pBitmapFromResource);
}

void MyApp::Render()
{
	if (!CreateDeviceResource()) return;

	pRenderTarget->BeginDraw();
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	// pBitmapFromFile을 그림.
	D2D1_SIZE_F size = pBitmapFromFile->GetSize();
	pRenderTarget->DrawBitmap(pBitmapFromFile, D2D1::RectF(0.0f, 0.0f, size.width, size.height));

	// pBitmapFromResource을 그림.
	pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(310, 200));
	size = pBitmapFromResource->GetSize();
	pRenderTarget->DrawBitmap(pBitmapFromResource, D2D1::RectF(0.0f, 0.0f, size.width, size.height));

	HRESULT hr = pRenderTarget->EndDraw();
	if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardDeviceResource();
}

// !{! LoadBitmapFromFile 함수 구현 코드. 원본 코드의 위치는 LoadBitmap/main.cpp.
HRESULT LoadBitmapFromFile(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR uri, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap)
{
	// 이 함수는 파일로부터 이미지를 로드하여 D2D 비트맵 ID2D1Bitmap 객체를 생성함.
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(uri, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
	if (FAILED(hr)) return hr;

	// 첫 번째 프레임을 얻음.
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) return hr;

	// 이미지 포맷을 32bppPBGRA로 변환함 (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = pIWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return hr;

	// 너비와 높이 중에 하나 이상이 명시되어 있으면, IWICBitmapScaler를 생성하고 이를 사용해서 이미지를 리사이즈함.
	if (destinationWidth != 0 || destinationHeight != 0)
	{
		UINT originalWidth, originalHeight;
		hr = pSource->GetSize(&originalWidth, &originalHeight);
		if (FAILED(hr)) return hr;

		if (destinationWidth == 0) // 높이가 명시된 값으로 정해지고, 너비는 비율만큼 자동으로 계산됨.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
			destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
		}
		else if (destinationHeight == 0) // 너비가 명시된 값으로 정해지고, 높이는 비율만큼 자동으로 계산됨.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
			destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
		}

		hr = pIWICFactory->CreateBitmapScaler(&pScaler);
		if (FAILED(hr)) return hr;

		hr = pScaler->Initialize(pSource, destinationWidth, destinationHeight, WICBitmapInterpolationModeCubic);
		if (FAILED(hr)) return hr;

		hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}
	else // 너비와 높이가 모두 0이면, 크기 변화가 없음을 의미하며, 이미지를 스케일하지 않음.
	{
		hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}

	// WIC 비트맵으로부터 D2D 비트맵인 ID2D1Bitmap 객체를 생성함.
	hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
	if (FAILED(hr)) return hr;

	SAFE_RELEASE(pDecoder);
	SAFE_RELEASE(pSource);
	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pConverter);
	SAFE_RELEASE(pScaler);

	return hr;
}
// !}!

// !{! LoadBitmapFromResource 함수 구현 코드. 원본 코드의 위치는 LoadBitmap/main.cpp.
HRESULT LoadBitmapFromResource(ID2D1RenderTarget* pRenderTarget, IWICImagingFactory* pIWICFactory, PCWSTR resourceName, PCWSTR resourceType, UINT destinationWidth, UINT destinationHeight, ID2D1Bitmap** ppBitmap)
{
	// 이 함수는 응용 프로그램의 리소스로부터 이미지를 로드하여 D2D 비트맵 ID2D1Bitmap 객체를 생성함.
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;

	HRSRC imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void* pImageFile = NULL;
	DWORD imageFileSize = 0;

	// 리소스를 찾음.
	imageResHandle = FindResourceW(NULL, resourceName, resourceType);
	HRESULT hr = imageResHandle ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 리소스를 로드함.
	imageResDataHandle = LoadResource(NULL, imageResHandle);
	hr = imageResDataHandle ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 리소스를 잠그고 메모리 포인터를 얻어옴.
	pImageFile = LockResource(imageResDataHandle);
	hr = pImageFile ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 이미지 크기를 계산함
	imageFileSize = SizeofResource(NULL, imageResHandle);
	hr = imageFileSize ? S_OK : E_FAIL;
	if (FAILED(hr)) return hr;

	// 리소스를 메모리에 매핑하기 위해서 WIC 스트림 객체를 생성함.
	hr = pIWICFactory->CreateStream(&pStream);
	if (FAILED(hr)) return hr;

	// 스트림을 초기화함.
	hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
	if (FAILED(hr)) return hr;

	// 스트림에 대한 디코더를 생성함.
	hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
	if (FAILED(hr)) return hr;

	// 첫 번째 프레임을 얻음.
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) return hr;

	// 이미지 포맷을 32bppPBGRA로 변환함 (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
	hr = pIWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) return hr;

	// 너비와 높이 중에 하나 이상이 명시되어 있으면, IWICBitmapScaler를 생성하고 이를 사용해서 이미지를 리사이즈함.
	if (destinationWidth != 0 || destinationHeight != 0)
	{
		UINT originalWidth, originalHeight;
		hr = pSource->GetSize(&originalWidth, &originalHeight);
		if (FAILED(hr)) return hr;

		if (destinationWidth == 0) // 높이가 명시된 값으로 정해지고, 너비는 비율만큼 자동으로 계산됨.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
			destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
		}
		else if (destinationHeight == 0) // 너비가 명시된 값으로 정해지고, 높이는 비율만큼 자동으로 계산됨.
		{
			FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
			destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
		}

		hr = pIWICFactory->CreateBitmapScaler(&pScaler);
		if (FAILED(hr)) return hr;

		hr = pScaler->Initialize(pSource, destinationWidth, destinationHeight, WICBitmapInterpolationModeCubic);
		if (FAILED(hr)) return hr;

		hr = pConverter->Initialize(pScaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}
	else // 너비와 높이가 모두 0이면, 크기 변화가 없음을 의미하며, 이미지를 스케일하지 않고 원본 크기로 생성함.
	{
		hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);
		if (FAILED(hr)) return hr;
	}

	// WIC 비트맵으로부터 D2D 비트맵인 ID2D1Bitmap 객체를 생성함.
	hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
	if (FAILED(hr)) return hr;

	SAFE_RELEASE(pDecoder);
	SAFE_RELEASE(pSource);
	SAFE_RELEASE(pStream);
	SAFE_RELEASE(pConverter);
	SAFE_RELEASE(pScaler);

	return hr;
}
// !}!
