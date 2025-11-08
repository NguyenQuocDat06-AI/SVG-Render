#include "readsvg.h"
#include "svgdocument.h"
#include "rect.h"
#include "polygon.h"
#include "ellipse.h"
#include "circle.h"
#include "text.h"
#include "line.h"
#include "polyline.h"
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
static inline std::string GetAttrRaw(READSVG& R, int i, const char* key) {
    auto att = R.GetAttributes(i);
    auto it = att.find(key);
    return (it == att.end()) ? std::string() : it->second;
}
// tìm trong style="...": prop có dạng "fill:" / "stroke:"
static inline std::string FindInStyle(READSVG& R, int i, const char* prop) {
    auto att = R.GetAttributes(i);
    auto it = att.find("style");
    if (it == att.end()) return {};
    const std::string& s = it->second;
    auto p = s.find(prop);
    if (p == std::string::npos) return {};
    p += std::strlen(prop);
    auto q = s.find(';', p);
    std::string v = s.substr(p, (q == std::string::npos) ? std::string::npos : q - p);
    // trim nhẹ
    auto l = v.find_first_not_of(" \t\r\n");
    auto r = v.find_last_not_of(" \t\r\n");
    return (l == std::string::npos) ? std::string() : v.substr(l, r - l + 1);
}
static inline bool IsFillNone(READSVG& R, int i) {
    std::string v = FindInStyle(R, i, "fill:");
    if (v.empty()) v = GetAttrRaw(R, i, "fill");
    // so sánh lower-case
    for (auto& c : v) c = (char)tolower((unsigned char)c);
    return v == "none";
}
static inline BYTE clamp255(int v) {
    if (v < 0) v = 0; else if (v > 255) v = 255;
    return (BYTE)v;
}

static void ApplyCommonPaint(SVGSHAPE* shp, READSVG& R, int i) {
    // display / visibility / opacity
    if (R.GetDisplayNone(i)) shp->SetDisplayNone(true);
    shp->SetVisible(R.GetVisible(i));
    shp->SetOverallOpacity(R.GetOpacity(i));

    const std::string tag = R.GetTagName(i);

    // ===== FILL =====
    {
        auto fill = R.GetFill(i);     // rỗng nếu không có hoặc 'none' (tùy cách bạn parse)
        if (!fill.empty()) {
            shp->EnableFill(clamp255(fill[0]), clamp255(fill[1]), clamp255(fill[2]));
            shp->SetFillOpacity(R.GetFillOpacity(i));
        }
        else {
            // Nếu không có fill nhưng cũng không phải fill="none" → áp mặc định theo SVG
            if (!IsFillNone(R, i)) {
                // SVG default fill = black cho mọi shape có diện tích (rect/circle/ellipse/polygon/polyline/path/text)
                // (line không có diện tích nên fill không có tác dụng, nhưng áp cũng không gây lỗi)
                shp->EnableFill(0, 0, 0);
                shp->SetFillOpacity(R.GetFillOpacity(i)); // dùng fill-opacity nếu có (vd: 0.5)
            }
            else {
                shp->DisableFill();
            }
        }
    }

    // ===== STROKE ===== (SVG default stroke = none)
    {
        auto stroke = R.GetStroke(i);
        float sw = R.GetStrokeWidth(i);

        // Với <text>: chỉ stroke khi thực có stroke
        if (tag == "text" && stroke.empty()) {
            shp->DisableStroke();
        }
        else if (!stroke.empty() && sw > 0.f) {
            shp->EnableStroke(clamp255(stroke[0]), clamp255(stroke[1]), clamp255(stroke[2]), sw);
            shp->SetStrokeOpacity(R.GetStrokeOpacity(i));
        }
        else {
            shp->DisableStroke();
        }
    }
}
// ===== hàm chính: đọc file & bơm shape vào g_doc =====
void LoadSvgToDocument(const std::string& path) {
    READSVG R;
    R.ParseFromBuffer(path);

    const int n = R.GetNodeCount();
    for (int i = 0; i < n; ++i) {
        const std::string tag = R.GetTagName(i);

        if (tag == "rect") {
            float x = R.GetRectX(i);
            float y = R.GetRectY(i);
            float w = R.GetRectWidth(i);
            float h = R.GetRectHeight(i);
            float rx = R.GetRectRx(i);
            float ry = R.GetRectRy(i);

            auto* s = new SVGRECT(x, y, w, h, rx, ry);
            ApplyCommonPaint(s, R, i);
            g_doc.AddShape(s);
        }
        else if (tag == "circle") {
            float cx = R.GetCx(i), cy = R.GetCy(i), r = R.GetR(i);
            auto* s = new SVGCIRCLE(cx, cy, r);
            ApplyCommonPaint(s, R, i);
            g_doc.AddShape(s);
        }
        else if (tag == "ellipse") {
            float cx = R.GetCx(i), cy = R.GetCy(i), rx = R.GetRx(i), ry = R.GetRy(i);
            auto* s = new SVGELLIPSE(cx, cy, rx, ry);
            ApplyCommonPaint(s, R, i);
            g_doc.AddShape(s);
        }
        else if (tag == "line") {
            float x1 = R.GetX1(i), y1 = R.GetY1(i), x2 = R.GetX2(i), y2 = R.GetY2(i);
            auto* s = new SVGLINE(x1, y1, x2, y2);
            ApplyCommonPaint(s, R, i);
            g_doc.AddShape(s);
        }
        else if (tag == "polyline") {
            auto pts = R.GetPoints(i);
            auto* s = new SVGPOLYLINE();
            for (auto& p : pts) s->AddPoint(p.first, p.second);
            ApplyCommonPaint(s, R, i);
            g_doc.AddShape(s);
        }
        else if (tag == "polygon") {
            auto pts = R.GetPoints(i);
            auto* s = new SVGPOLYGON();
            for (auto& p : pts) s->AddPoint(p.first, p.second);
            ApplyCommonPaint(s, R, i);
            g_doc.AddShape(s);
        }
        else if (tag == "text") {
            float x = R.GetX(i), y = R.GetY(i);
            std::string content = R.GetNode(i)._text;               // nội dung giữa thẻ
            auto* s = new SVGTEXT(x, y, content);

            // font/layout
            s->SetFont(R.GetFontFamily(i), R.GetFontSize(i));
            s->SetStyle(R.GetFontWeight(i), R.GetFontStyle(i));
            s->SetAnchor(R.GetTextAnchor(i));

            // paint
            ApplyCommonPaint(s, R, i);
            g_doc.AddShape(s);
        }
        // else if (tag == "path") {  // TODO: cần parser path 'd'
        //     std::string d = R.GetPathD(i);
        //     auto* s = new SVGPATH(d); // nếu bạn có lớp này
        //     ApplyCommonPaint(s, R, i);
        //     g_doc.AddShape(s);
        // }
        // các tag khác (defs, g, use, image, ...) tùy bạn bổ sung dần
    }
}

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
<<<<<<< HEAD
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
    g_doc.AddShape(polygon);

=======
    LoadSvgToDocument("sample.svg");
>>>>>>> origin/main
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