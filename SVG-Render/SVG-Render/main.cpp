#include "readsvg.h"
#include "svgdocument.h"
using namespace Gdiplus;
// ============================================================================
// DEBUG MAIN // PROJECT -> LINKER -> SUBSYSTEM -> Console (/SUBSYSTEM:CONSOLE)
// ============================================================================

//int main() {
//	READSVG _svg;
//	_svg.ParseFromBuffer("sample.svg");
//	_svg.PrintNode();
//	vector<int> rgb(_svg.GetFill(1));
//	cout << endl;
//	for (int c : rgb) {
//		cout << c << " ";
//	}
//	cout << endl;
//	float op = _svg.GetFillOpacity(0);
//	cout << op << endl;
//	float h = _svg.GetHeight(0);
//	cout << h << endl;
//	rgb = _svg.GetStroke(2);
//	for (int c : rgb) {
//		cout << c << " ";
//	}
//}
// ============================================================================
// Global PROJECT -> LINKER -> SUBSYSTEM -> Windows (/SUBSYSTEM:WINDOWS)
// ============================================================================
ULONG_PTR g_GdiToken;
SVGDOCUMENT g_doc;
// ===== helper nhỏ =====

// tìm trong style="...": prop có dạng "fill:" / "stroke:"

// ===== hàm chính: đọc file & bơm shape vào g_doc =====

// ============================================================================
// WndProc
// ============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch (message)
    {
    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);
        Gdiplus::Graphics g(hdc);
		RECT rc;
		GetClientRect(hWnd, &rc); // lấy kích thước cửa sổ
        g_doc.Render(g,rc.right-rc.left,rc.bottom-rc.top);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) {
            g_doc.ZoomIn();
        }
        else if (delta < 0) {
            g_doc.ZoomOut();
        }
        InvalidateRect(hWnd, NULL, FALSE);
        break;
	}
    case WM_KEYDOWN:
    {
        switch (wParam) {
            case VK_LEFT:
                g_doc.RotateLeft();
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            case VK_RIGHT:
                g_doc.RotateRight();
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            default:
                break;
        }
		break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
} // WndProc

// ============================================================================
// WinMain
// ============================================================================
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
    // ============================================================================
    // CODE ... rect, text, circle, polyline, ellipse, line, polygon 
   
    //
    g_doc.LoadSvgToDocument("sample.svg");
    // g_doc.AddShape(...);
    // ============================================================================
    HWND                hWnd;
    MSG                 msg;
    WNDCLASS            wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = TEXT("GettingStarted");

    RegisterClass(&wndClass);

    hWnd = CreateWindow(
        TEXT("GettingStarted"),   // window class name
        TEXT("SVG Demo"),  // window caption
        WS_OVERLAPPEDWINDOW,      // window style
        CW_USEDEFAULT,            // initial x position
        CW_USEDEFAULT,            // initial y position
        CW_USEDEFAULT,            // initial x size
        CW_USEDEFAULT,            // initial y size
        NULL,                     // parent window handle
        NULL,                     // window menu handle
        hInstance,                // program instance handle
        NULL);                    // creation parameters

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}  // WinMain