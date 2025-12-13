#include "readsvg.h"
#include "svgdocument.h"
#include <iostream>
#include <string>
#include <windows.h> // Can thiet cho WinMain va cac API
#include <shellapi.h> // Can thiet cho CommandLineToArgvW

using namespace Gdiplus;

// Bien toan cuc de luu duong dan file SVG
std::string g_svgFilePath;

ULONG_PTR g_GdiToken;
SVGDOCUMENT* g_doc = nullptr;
// ... (WndProc giu nguyen) ...

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
        if (g_doc) {
            RECT rect;
            GetClientRect(hWnd, &rect);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // Goi ham Render
            g_doc->Render(g, width, height);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}


INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
    // =========================================================
    // BƯỚC SỬA: XỬ LÝ COMMAND LINE ARGUMENT
    // =========================================================
    int argc = 0;
    // Lay chuoi Command Line (WCHAR*)
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

    // Kiem tra so luong doi so: phai co ten chuong trinh (argv[0]) va duong dan file (argv[1])
    if (argc != 2) {
        // Hien thi thong bao loi
        MessageBox(NULL, L"Loi: Vui long cung cap duong dan den file SVG.",
            L"Loi Command Line", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Chuyen WCHAR* (Unicode) cua argv[1] sang std::string
    // (Mac du duong dan tren Windows nen dung WCHAR*, nhung de tuong thich code cu:
    std::wstring ws(argvW[1]);
    g_svgFilePath.assign(ws.begin(), ws.end());

    LocalFree(argvW); // Giai phong bo nho duoc cap phat boi CommandLineToArgvW

    // =========================================================
    // KHOI TAO VÀ LOAD FILE
    // =========================================================
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    g_doc = new SVGDOCUMENT();

    try {
        // Goi ham load voi duong dan da lay tu command line
        g_doc->LoadSvgToDocument(g_svgFilePath);
    }
    catch (const std::runtime_error& e) {
        // Hien thi loi doc file SVG
        std::string errorMessage = "Loi khi doc file SVG: " + std::string(e.what());
        MessageBoxA(NULL, errorMessage.c_str(), "Loi Doc File", MB_ICONERROR | MB_OK);
        delete g_doc;
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    HWND                hWnd;
    MSG                 msg;
    WNDCLASS            wndClass;

    // ... (Phan khoi tao cua so Win32 giu nguyen) ...
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
        TEXT("SVG Demo"),         // window caption
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

    if (g_doc) {
        delete g_doc;
        g_doc = nullptr;
    }

    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}