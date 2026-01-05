#include "readsvg.h"
#include "svgdocument.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>

// Link thư viện Gdiplus
#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

// --- ĐỊNH NGHĨA ID CHO UI ---
#define ID_BUTTON_FILE  101
#define ID_MENU_FIRST   200
#define ID_MENU_MANUAL  199
// --- BIẾN TOÀN CỤC ---
std::string g_svgFilePath;
ULONG_PTR g_GdiToken;
SVGDOCUMENT* g_doc = nullptr;
HWND g_hButtonFile = NULL;
std::vector<std::string> g_fileList;

// --- HÀM HỖ TRỢ: LOAD FILE SVG ---
void LoadSVGFile(const std::string& path, HWND hWnd) {
    if (g_doc) {
        delete g_doc;
        g_doc = nullptr;
    }

    g_doc = new SVGDOCUMENT();
    try {
        g_doc->LoadSvgToDocument(path);
        g_svgFilePath = path;

        std::string title = "SVG Demo - " + path;
        SetWindowTextA(hWnd, title.c_str());

        InvalidateRect(hWnd, NULL, FALSE);
    }
    catch (const std::exception& e) {
        std::string msg = "Loi doc file: " + std::string(e.what());
        MessageBoxA(hWnd, msg.c_str(), "Error", MB_ICONERROR);
        delete g_doc;
        g_doc = nullptr;
    }
}

// --- HÀM HỖ TRỢ: QUÉT FILE SVG (DÙNG WIN32 API CHO) ---
void ScanSVGFiles() {
    g_fileList.clear();

    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exePath(buffer);

    std::string exeDir = "";
    size_t lastSlash = exePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        exeDir = exePath.substr(0, lastSlash + 1);
    }

    std::string searchPattern = exeDir + "*.svg";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                g_fileList.push_back(exeDir + findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch (message)
    {
    case WM_CREATE:
    {
        g_hButtonFile = CreateWindow(
            TEXT("BUTTON"), TEXT("Chon File SVG"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 10, 120, 30,
            hWnd, (HMENU)ID_BUTTON_FILE,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
        return 0;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == ID_BUTTON_FILE) {
            ScanSVGFiles();
            HMENU hPopupMenu = CreatePopupMenu();

            AppendMenuA(hPopupMenu, MF_STRING, ID_MENU_MANUAL, "[+] Mo file tu duong dan khac...");
            AppendMenuA(hPopupMenu, MF_SEPARATOR, 0, NULL); // Đường kẻ ngang phân cách
            // ------------------------------------------------

            if (g_fileList.empty()) {
                // Nếu không có file nào trong thư mục, chỉ hiện dòng "Mở file khác" rồi disable các dòng dưới
                AppendMenuA(hPopupMenu, MF_STRING | MF_GRAYED, 0, "(Khong tim thay file .svg tai folder nay)");
            }
            else {
                // Nếu có file, liệt kê ra như cũ
                for (size_t i = 0; i < g_fileList.size(); ++i) {
                    std::string fullPath = g_fileList[i];
                    std::string displayName = fullPath;

                    size_t lastSlash = fullPath.find_last_of("\\/");
                    if (lastSlash != std::string::npos) {
                        displayName = fullPath.substr(lastSlash + 1);
                    }
                    AppendMenuA(hPopupMenu, MF_STRING, ID_MENU_FIRST + i, displayName.c_str());
                }
            }

            RECT rect;
            GetWindowRect(g_hButtonFile, &rect);
            TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, rect.left, rect.bottom, 0, hWnd, NULL);
            DestroyMenu(hPopupMenu);
        }

        // 2. [THÊM MỚI] XỬ LÝ KHI CHỌN "MO FILE TU DUONG DAN KHAC"
        else if (wmId == ID_MENU_MANUAL) {
            OPENFILENAMEA ofn;     
            char szFile[260];        // Buffer để chứa tên file
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "SVG Files\0*.svg\0All Files\0*.*\0"; // Chỉ lọc file SVG
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            // Hiện hộp thoại Open File chuẩn của Windows
            if (GetOpenFileNameA(&ofn) == TRUE) {
                // Nếu người dùng chọn file và nhấn OK
                LoadSVGFile(ofn.lpstrFile, hWnd);
            }

            SetFocus(hWnd); // Lấy lại tiêu điểm để phím tắt hoạt động
        }

        // 3. XỬ LÝ KHI CHỌN FILE TRONG DANH SÁCH QUÉT ĐƯỢC
        else if (wmId >= ID_MENU_FIRST && wmId < ID_MENU_FIRST + (int)g_fileList.size()) {
            int index = wmId - ID_MENU_FIRST;
            std::string selectedFile = g_fileList[index];
            LoadSVGFile(selectedFile, hWnd);
            SetFocus(hWnd);
        }
        break;
    }

    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // --- BẮT ĐẦU DOUBLE BUFFERING ---

        // 1. Tạo một Bitmap ảo trong bộ nhớ có kích thước bằng màn hình
        Gdiplus::Bitmap bufferBitmap(width, height, PixelFormat32bppARGB);

        // 2. Tạo Graphics để vẽ lên Bitmap ảo đó (chứ không vẽ lên màn hình ngay)
        Gdiplus::Graphics g(&bufferBitmap);

        // Tùy chỉnh chất lượng vẽ cho mượt (Anti-aliasing)
        g.SetSmoothingMode(SmoothingModeAntiAlias);

        // 3. Xóa nền trên Bitmap ảo
        g.Clear(Color(255, 255, 255, 255));

        // 4. Vẽ SVG lên Bitmap ảo
        if (g_doc) {
            g_doc->Render(g, width, height);
        }

        // 5. Reset Transform để vẽ UI (như đã sửa ở bước trước)
        g.ResetTransform();

        Gdiplus::Font font(L"Arial", 10, FontStyleBold);
        Gdiplus::SolidBrush brush(Color(200, 0, 0, 0));
        Gdiplus::SolidBrush bgBrush(Color(100, 255, 255, 255));

        std::wstring infoText =
            L"Dieu khien:\n"
            L"- Lan chuot: Phong to / Thu nho\n"
            L"- W/A/S/D: Di chuyen\n"
            L"- Mui ten Trai/Phai: Xoay hinh";

        g.FillRectangle(&bgBrush, 10, height - 80, 250, 70);
        g.DrawString(infoText.c_str(), -1, &font, PointF(15.0f, (REAL)(height - 75)), &brush);

        if (!g_doc) {
            Gdiplus::Font bigFont(L"Arial", 16);
            std::wstring msg = L"Vui long nhan nut 'Chon File SVG' de bat dau";
            g.DrawString(msg.c_str(), -1, &bigFont, PointF((REAL)(width / 2 - 150), (REAL)(height / 2)), &brush);
        }

        // --- KẾT THÚC VẼ TRÊN BỘ NHỚ ---

        // 7. Copy toàn bộ Bitmap ảo ra màn hình thật
        Gdiplus::Graphics screenGraphics(hdc);
        screenGraphics.DrawImage(&bufferBitmap, 0, 0);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEWHEEL:
    {
        if (g_doc) {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0) g_doc->ZoomIn();
            else if (delta < 0) g_doc->ZoomOut();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }

    case WM_KEYDOWN:
    {
        if (g_doc) {
            switch (wParam) {
            case VK_LEFT:  g_doc->RotateLeft(); break;
            case VK_RIGHT: g_doc->RotateRight(); break;
            case 'A':      g_doc->MoveLeft(); break;
            case 'D':      g_doc->MoveRight(); break;
            case 'W':      g_doc->MoveUp(); break;
            case 'S':      g_doc->MoveDown(); break;
            }
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_GdiToken, &gdiplusStartupInput, NULL);

    int argc = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::string initialFile = "";
    if (argc == 2) {
        std::wstring ws(argvW[1]);
        initialFile.assign(ws.begin(), ws.end());
    }
    LocalFree(argvW);

    WNDCLASS wndClass = { 0 };
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszClassName = TEXT("SVGReaderClass");

    RegisterClass(&wndClass);

    HWND hWnd = CreateWindow(
        TEXT("SVGReaderClass"),
        TEXT("SVG Viewer Pro"),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    if (!initialFile.empty()) {
        LoadSVGFile(initialFile, hWnd);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_doc) {
        delete g_doc;
        g_doc = nullptr;
    }
    GdiplusShutdown(g_GdiToken);
    return (int)msg.wParam;
}