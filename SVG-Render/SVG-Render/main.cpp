#include "readsvg.h"
#include "svgdocument.h"
#include "rect.h"
#include "polygon.h"
#include "ellipse.h"
#include "circle.h"
using namespace Gdiplus;
// ============================================================================
// DEBUG MAIN // PROJECT -> LINKER -> SUBSYSTEM -> Console (/SUBSYSTEM:CONSOLE)
// ============================================================================

//int main() {
//	READSVG _svg;
//	_svg.ParseFromBuffer("sample.svg");
//	_svg.PrintNode();
//}

// ============================================================================
// Global PROJECT -> LINKER -> SUBSYSTEM -> Windows (/SUBSYSTEM:WINDOWS)
// ============================================================================
ULONG_PTR g_GdiToken;
SVGDOCUMENT g_doc;

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
        g_doc.Render(g);
        EndPaint(hWnd, &ps);
        return 0;
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
	SVGRECT* rect = new SVGRECT(20,20,800,400,0,0);
	rect->EnableFill(200, 100, 150); // Red fill
	rect->SetFillOpacity(0.2f);
	rect->EnableStroke(555, 55, 55, 2.0f); // Black stroke

	g_doc.AddShape(rect);

	SVGRECT* rect2 = new SVGRECT(0, 0, 200, 50, 0.0f, 0.0f);
    rect2->SetFillOpacity(0.0f);
    rect2->EnableStroke(255, 0, 0, 2.0f);
	g_doc.AddShape(rect2);


	SVGPOLYGON* polygon = new SVGPOLYGON();

    //350,75 379,161 469,161 397,215 423,301 350,250 277,301 303,215 231,161 321,161
	polygon->AddPoint(350.0f, 75.0f);
	polygon->AddPoint(379.0f, 161.0f);
	polygon->AddPoint(469.0f, 161.0f);
	polygon->AddPoint(397.0f, 215.0f);
	polygon->AddPoint(423.0f, 301.0f);
	polygon->AddPoint(350.0f, 250.0f);
	polygon->AddPoint(277.0f, 301.0f);
	polygon->AddPoint(303.0f, 215.0f);
	polygon->AddPoint(231.0f, 161.0f);
	polygon->AddPoint(321.0f, 161.0f);
	polygon->EnableFill(255, 255, 0);
	polygon->SetFillOpacity(0.6f);
	polygon->EnableStroke(250, 0, 0, 10.0f);

	SVGPOLYGON* polygon2 = new SVGPOLYGON();
    //points="850,75 958,137 958,262 850,325 742,262 742,137"
	polygon2->AddPoint(850.0f, 75.0f);
	polygon2->AddPoint(958.0f, 137.0f);
	polygon2->AddPoint(958.0f, 262.0f);
	polygon2->AddPoint(850.0f, 325.0f);
	polygon2->AddPoint(742.0f, 262.0f);
	polygon2->AddPoint(742.0f, 137.0f);
    polygon2->EnableFill(153, 204, 255);
	polygon2->SetFillOpacity(0.5f);
	polygon2->EnableStroke(255, 0, 102, 10.0f);

	g_doc.AddShape(polygon);
	g_doc.AddShape(polygon2);


    SVGELLIPSE* ellipse = new SVGELLIPSE(500.0f, 100.0f, 100.0f, 50.0f);
    ellipse->EnableFill(0, 255, 0); // Green fill
    ellipse->SetFillOpacity(0.5f);
    ellipse->EnableStroke(255, 255, 0, 3.0f); // Yellow stroke
    ellipse->SetStrokeOpacity(0.7f);
    g_doc.AddShape(ellipse);

	SVGCIRCLE* circle = new SVGCIRCLE(200.0f, 300.0f, 100.0f);
	circle->EnableFill(255, 255, 0); // Yellow fill
	circle->SetFillOpacity(0.5f);
	circle->EnableStroke(0, 255, 255, 10.0f); // Cyan stroke
	circle->SetStrokeOpacity(0.7f);
	g_doc.AddShape(circle);

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