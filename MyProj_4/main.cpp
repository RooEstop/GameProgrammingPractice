#include <Windows.h>

#include <tchar.h>
#if defined(_DEBUG)&&defined(_WINDOWS)&&!defined(_AFX)&&!defined(_AFXDLL)
#define TRACE TRACE_WINDOWS
#pragma warning(disable: 4996)
void TRACE_WINDOWS(LPCTSTR lpszFormat, ...) {
	TCHAR lpszBuffer[0x160]; //buffer size
	va_list fmtList;
	va_start(fmtList, lpszFormat);
	_vstprintf(lpszBuffer, lpszFormat, fmtList);
	va_end(fmtList);
	OutputDebugString(lpszBuffer);
}
#endif


HWND hWnd = NULL;
HRESULT initWin(HINSTANCE instanceHandle, int show);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


//Entry Point.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	if (FAILED(initWin(hInstance, nCmdShow)))
	{
		MessageBox(0, L"initWin - Failed", L"Error", MB_OK);
		return FALSE;
	}

	//Accept all input action from user. GetMessage return 0 when window is closed.
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

HRESULT initWin(HINSTANCE hInstance, int nCmdShow)
{
	//WIndows Class Init
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;	//Window Procedure
	wc.hInstance = hInstance;
	wc.lpszClassName = L"Hello";

	//RegisterClass() : Register WNDClass as class
	if (!RegisterClass(&wc)) {
		MessageBox(0, L"RegisterClass - Failed", 0, 0);
		return E_FAIL;
	}

	//CreateWindow : Create Window. Return initialize value to hWnd.
	hWnd = CreateWindow(L"Hello", L"Hello", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		MessageBox(0, L"CreateWindow - Failed", 0, 0);
		return E_FAIL;
	}

	//Show window on screen.
	ShowWindow(hWnd, nCmdShow);

	//Update window state.
	UpdateWindow(hWnd);

	return S_OK;
}

LRESULT OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	//GetClientRect : Get Window size of Client. Save left, top, right, bottom coordinate.
	//left, top => 0, 0
	//right => width, bottom => height
	RECT rc;
	GetClientRect(hWnd, &rc);

	FillRect(hdc, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));

	//Save Old Obj to restore
	HPEN hMyRedPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
	HGDIOBJ hOldPen = SelectObject(hdc, hMyRedPen);

	//Draw Rectangle with line.(Not filled)
	Rectangle(hdc, rc.left + 100, rc.top + 100, rc.right - 100, rc.bottom - 100);

	//restore origin object
	SelectObject(hdc, hOldPen);
	//Release HPEN object
	DeleteObject(hMyRedPen);

	SetTextAlign(ps.hdc, TA_CENTER);

	const TCHAR message[] = L"안녕하세요!";
	TextOut(hdc, (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2, message, lstrlen(message));

	//Release PaintStruct
	EndPaint(hWnd, &ps);
	return 0;
}

//Window Procedure Implement. Parameter is Fixed.
//Processing message (Event Handler)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WCHAR strbuf[180];
	int MBResult = 0;
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
		TRACE(L"좌클릭 감지, X : %d Y : %d\n", LOWORD(lParam), HIWORD(lParam));
		MessageBox(hWnd, L"왼쪽 버튼을 눌렀습니다.", L"Alarm", MB_OK | MB_ICONINFORMATION);
		return 0;
	case WM_RBUTTONDOWN:
		MBResult = MessageBox(hWnd, L"오류가 발생했습니다. 다시 시도할까요?", L"Error", MB_RETRYCANCEL | MB_ICONERROR);
		switch (MBResult)
		{
		case IDRETRY:
			MessageBox(hWnd, L"다시 시도를 선택했습니다.", L"Alarm", MB_OK | MB_ICONINFORMATION);
			return 0;
		case IDCANCEL:
			MessageBox(hWnd, L"취소를 선택했습니다.", L"Alarm", MB_OK | MB_ICONINFORMATION);
			return 0;

		default:
			TRACE(L"Select Noting");
			return 0;
		}
		return 0;
	case WM_PAINT:
		OnPaint(hWnd);
		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}