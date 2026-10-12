#include <windows.h> // 이 헤더파일은 Win32 API 관련 구조와 함수 선언들을 포함함.

HWND hWnd = NULL; // 메인 윈도우 핸들.

HRESULT initWin(HINSTANCE instanceHandle, int show);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 응용 프로그램 진입 함수.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	if (FAILED(initWin(hInstance, nCmdShow)))
	{
		MessageBox(0, L"initWin - Failed", L"Error", MB_OK);
		return FALSE;
	}

	// 메시지 루프
	// 윈도우가 생성된 후 메시지 루프에 진입하고 WM_QUIT 메시지가 올 때까지 메시지 루프를 반복함.
	MSG msg = {};
	// GetMessage 함수는 메시지 큐에 메시지가 들어올 떄까지 블록되어 기다림.
	// GetMessage 함수는 들어온 메시지가 WM_QUIT 일 때에만 0을 되돌림. 이때 루프를 벗어나도록 하자.
	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		TranslateMessage(&msg); //윈도우가 생성한 가상키 코드를 문자메시지로 해석함
		DispatchMessage(&msg); //메시지를 window procedure로 보냄
	}

	return (int)msg.wParam;
}

// 초기화가 성공적으로 수행되면 true를 되돌림.
HRESULT initWin(HINSTANCE hInstance, int nCmdShow)
{
	// WNDCLASS 구조체에 값을 채워서 생성하고자 할 윈도우의 특성을 지정함.
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"Hello";

	// 윈도우 클래스 description을 등록함.
	if (!RegisterClass(&wc)) {
		MessageBox(0, L"RegisterClass - Failed", 0, 0);
		return E_FAIL;
	}

	// 등록된 윈도우 클래스 description에 기반하여 윈도우를 생성함.
	// 생성된 윈도우의 HWND를 되돌림.
	hWnd = CreateWindow(L"Hello", L"Hello", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		MessageBox(0, L"CreateWindow - Failed", 0, 0);
		return E_FAIL;
	}

	// 생성된 윈도우를 show하고 update함.
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return S_OK;
}

LRESULT OnPaint(HWND hWnd);

// 전역 변수 선언
// 키보드
int codeVK = 0;
int codeAscii = 0;
// 마우스
POINT mousePosScreen = { 0 };

#include <string> //std::wstring
// 출력할 문자열
std::wstring str;
bool Update();

// 윈도우가 받는 이벤트들을 처리하는 윈도우 프로시져.
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
	{
		if (!Update())
		{
			DestroyWindow(hWnd);
			return 0;
		}

		OnPaint(hWnd);
		// 윈도우가 반복해서 화면을 그리도록 하려면 ValidateRect을 호출하지 말아야 함.
		return 0;
	}

	// 키보드 이벤트 처리

	case WM_CHAR:
	{
		codeAscii = (int)wParam;
		InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	}

	case WM_KEYDOWN:
	{
		if (wParam == VK_ESCAPE) // ESC 키를 누르면 종료함.
		{
			DestroyWindow(hWnd);
			return 0;
		}
		codeVK = (int)wParam;
		InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	}

	// 마우스 이벤트 처리

	case WM_MOUSEMOVE:
	{
		GetCursorPos(&mousePosScreen); // 마우스 위치를 받아온다
		InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		POINT pos;
		pos.x = 300; // 클라이언트 좌표계 기준으로 (300, 300)으로 설정
		pos.y = 300;
		ClientToScreen(hWnd, &pos); // 클라이언트 기준 좌표를 바탕화면 기준으로 변환한다
		SetCursorPos(pos.x, pos.y); // 커서를 윈도우 클라이언트 기준으로 (300, 300)에 위치시킨다
		InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	}

	case WM_RBUTTONDOWN:
	{
		POINT pos;
		pos.x = 300; // 스크린 좌표계 기준으로 (300, 300)으로 설정, 윈도우를 벗어날 수 있음
		pos.y = 300;
		SetCursorPos(pos.x, pos.y); // 커서를 스크린 기준으로 (300, 300)에 위치시킨다
		InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	}

	case WM_DESTROY:
		// destroy 메시지를 받으면 quit 메시지를 보내서 메시지 루프를 벗어나도록 함.
		PostQuitMessage(0);
		return 0;

	default:
		// 그 외의 메시지들은 디폴트 윈도우 프로시져로 forward함.
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

bool Update()
// !{! Windows 입력 예제 코드. 원본 코드의 위치는 WindowsInputSimple/main.cpp.
{
	// 응용 프로그램을 종료하는 경우에 false를 리턴하고 그 외에는 항상 true를 리턴함.

	// 'q' 키를 누르면 응용 프로그램을 종료함.
	// GetAsyncKeyState 함수는 'q'가 아닌 'Q', 항상 대문자를 사용해야 함.
	if (GetAsyncKeyState('Q') & 0x8000f) return false;

	// 입력 상태를 갱신함.

	WCHAR strbuf[180];

	str.clear();

	// 키보드

	// VK
	if (codeVK)
		wsprintf(strbuf, L"[VK]: 0x%04x %c", codeVK, codeVK);
	else
		wsprintf(strbuf, L"[VK]: unpressed");
	str += strbuf;
	str += L"\n";

	// ASCII
	if (codeAscii)
		wsprintf(strbuf, L"[ASCII]: 0x%04x %c", codeAscii, codeAscii);
	else
		wsprintf(strbuf, L"[ASCII]: unpressed");
	str += strbuf;
	str += L"\n\n";

	// GetKeyState
	bool isKeyDown = false;
	bool isKeyToggled = false;
	short keyState = GetKeyState(VK_SPACE);
	if (keyState & 0x8000) // MSB가 1이면, 키가 down임.
		isKeyDown = true;
	if (keyState & 0x0001) // LSB가 1이면, 키가 토글됨.
		isKeyToggled = true;
	wsprintf(strbuf, L"[GetKeyState]: VK_SPACE key: %s, %s",
		(isKeyDown ? L"down" : L"up"),
		(isKeyToggled ? L"toggled" : L"untoggled"));
	str += strbuf;
	str += L"\n";

	// GetAsyncKeyState
	bool isKeyDownCurrently = false;
	bool isKeyPressedAfterPreviousCall = false;
	keyState = GetAsyncKeyState(VK_SPACE);
	if (keyState & 0x8000) // MSB가 1이면, 키가 현재 down 상태임.
		isKeyDownCurrently = true;
	if (keyState & 0x0001) // LSB가 1이면, 직전 GetAsyncKeyState 함수의 호출 이후로 키가 눌러진적 있음.
		isKeyPressedAfterPreviousCall = true;
	wsprintf(strbuf, L"[GetAsyncKeyState]: VK_SPACE key: %s, %s",
		(isKeyDownCurrently ? L"down currently" : L"up currently"),
		(isKeyPressedAfterPreviousCall ? L"pressed after prev call" : L"unpressed after prev call"));
	str += strbuf;
	str += L"\n\n";

	// 마우스

	// 스크린 좌표계로 마우스 위치를 표시한다.
	wsprintf(strbuf, L"[screen coord]: X: %4d, Y: %4d", mousePosScreen.x, mousePosScreen.y);
	str += strbuf;
	str += L"\n";

	// 클라이언트 좌표계로 마우스 위치를 표시한다.
	POINT mousePosClient = mousePosScreen;
	ScreenToClient(hWnd, &mousePosClient); // 해당 클라이언트 기준으로 좌표를 변환한다
	wsprintf(strbuf, L"[client coord]: X: %4d, Y: %4d", mousePosClient.x, mousePosClient.y);
	str += strbuf;
	str += L"\n";

	return true;
}
// !}!

LRESULT OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	// 창 전체 클리어
	RECT rc;
	GetClientRect(hWnd, &rc);
	FillRect(hdc, &rc, GetSysColorBrush(COLOR_WINDOW));

	// 문자열을 출력한다.
	rc.left += 50;
	DrawText(hdc, str.c_str(), -1, &rc, DT_LEFT);

	EndPaint(hWnd, &ps);
	return 0;
}
