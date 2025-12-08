#include "svgshape.h"
#include <algorithm>
#include <cctype>

inline BYTE AlphaFromOpacity(float op) {
    if (op < 0.0f) op = 0.0f;
    if (op > 1.0f) op = 1.0f;
    return static_cast<BYTE>(op * 255.0f);
} // Chuyển sang 8 bit

// Helper function để chuyển đổi stroke-linecap từ string sang GDI+ enum
static Gdiplus::LineCap StringToLineCap(const std::string& cap) {
    std::string capLower = cap;
    std::transform(capLower.begin(), capLower.end(), capLower.begin(), 
        [](unsigned char c) { return std::tolower(c); });
    
    if (capLower == "round") return Gdiplus::LineCapRound;
    if (capLower == "square") return Gdiplus::LineCapSquare;
    return Gdiplus::LineCapFlat; // "butt" hoặc mặc định
}

// Helper function để chuyển đổi stroke-linejoin từ string sang GDI+ enum
static Gdiplus::LineJoin StringToLineJoin(const std::string& join) {
    std::string joinLower = join;
    std::transform(joinLower.begin(), joinLower.end(), joinLower.begin(), 
        [](unsigned char c) { return std::tolower(c); });
    
    if (joinLower == "round") return Gdiplus::LineJoinRound;
    if (joinLower == "bevel") return Gdiplus::LineJoinBevel;
    return Gdiplus::LineJoinMiter; // "miter" hoặc mặc định
}
SVGSHAPE::SVGSHAPE() {
    visible = true;
    displayNone = false;
    hasFill = false;
    fillColor = Gdiplus::Color(255, 0, 0, 0);
    fillOpacity = 1.0f;
    hasStroke = false;
    strokeColor = Gdiplus::Color(255, 0, 0, 0);
    strokeOpacity = 1.0f;
    strokeWidth = 1.0f;
    overallOpacity = 1.0f;
    hasTransform = false;
}
void SVGSHAPE::SetVisible(bool v) {
    visible = v;
}

void SVGSHAPE::SetDisplayNone(bool d) {
    displayNone = d;
}
void SVGSHAPE::EnableFill(BYTE r, BYTE g, BYTE b) {
    hasFill = true;
    fillColor = Gdiplus::Color(255, r, g, b); // alpha tạm đặt 255
}
void SVGSHAPE::DisableFill() {
    hasFill = false;
}
void SVGSHAPE::SetFillOpacity(float op) {
    fillOpacity = op;
}
void SVGSHAPE::EnableStroke(BYTE r, BYTE g, BYTE b, float width) {
    hasStroke = true;
    strokeColor = Gdiplus::Color(255, r, g, b); // alpha tạm đặt 255
    strokeWidth = width;
}
void SVGSHAPE::DisableStroke() {
    hasStroke = false;
}
void SVGSHAPE::SetStrokeOpacity(float op) {
    strokeOpacity = op;
}
void SVGSHAPE::SetStrokeWidth(float w) {
    strokeWidth = w;
}
void SVGSHAPE::SetOverallOpacity(float op) {
    overallOpacity = op;
}
void SVGSHAPE::SetStrokeMiterLimit(float m) {
    strokeMiterLimit = m;
}
void SVGSHAPE::SetFillRule(const std::string& rule) {
    fillRule = rule;
}
void SVGSHAPE::SetStrokeLinecap(const std::string& cap) {
    strokeLinecap = cap;
}
void SVGSHAPE::SetStrokeLinejoin(const std::string& join) {
    strokeLinejoin = join;
}

void SVGSHAPE::SetTransform(const Gdiplus::Matrix& mtx) {
    // Copy matrix bằng cách copy các elements
    REAL elements[6];
    mtx.GetElements(elements);
    transform.SetElements(elements[0], elements[1], elements[2], elements[3], elements[4], elements[5]);
    
	hasTransform = true;
}

void SVGSHAPE::Draw(Gdiplus::Graphics& g) const {
    if (!visible) return;
    if (displayNone) return;

    GraphicsContainer container = g.BeginContainer();


    if (hasTransform) {
        g.MultiplyTransform(&transform, Gdiplus::MatrixOrderAppend);
    }

    // 3. Tính toán Alpha
    BYTE finalFillAlpha = AlphaFromOpacity(fillOpacity * overallOpacity);
    BYTE finalStrokeAlpha = AlphaFromOpacity(strokeOpacity * overallOpacity);

    // 4. Vẽ thực sự (lớp con thực hiện)
    DrawImpl(g, finalFillAlpha, finalStrokeAlpha);

    // 5. Khôi phục trạng thái Graphics như cũ
    g.EndContainer(container);
}