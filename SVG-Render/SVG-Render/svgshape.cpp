#include "svgshape.h"
inline BYTE AlphaFromOpacity(float op) {
    if (op < 0.0f) op = 0.0f;
    if (op > 1.0f) op = 1.0f;
    return static_cast<BYTE>(op * 255.0f);
} // Chuyển sang 8 bit
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
void SVGSHAPE::Draw(Gdiplus::Graphics& g) const {
    if (!visible) return;
    if (displayNone) return;
    BYTE finalFillAlpha = AlphaFromOpacity(fillOpacity * overallOpacity);
    BYTE finalStrokeAlpha = AlphaFromOpacity(strokeOpacity * overallOpacity);
    DrawImpl(g, finalFillAlpha, finalStrokeAlpha);
}

SvgPolyline::SvgPolyline(const std::vector<PointF>& pts)
    : points(pts) {
}

void SvgPolyline::AddPoint(float x, float y) {
    points.emplace_back(x, y);
}

void SvgPolyline::SetPoints(const std::vector<PointF>& pts) {
    points = pts;
}

const std::vector<PointF>& SvgPolyline::GetPoints() const {
    return points;
}

void SvgPolyline::DrawImpl(Graphics& g, BYTE fillA, BYTE strokeA) const {
    if (points.size() < 2)
        return; // cần ít nhất 2 điểm để vẽ đường

    // === Fill (nếu có) ===
    if (hasFill && points.size() >= 3) {
        Color c(fillA, fillColor.GetR(), fillColor.GetG(), fillColor.GetB());
        SolidBrush brush(c);
        g.FillPolygon(&brush, points.data(), static_cast<INT>(points.size()));
    }

    // === Stroke (nếu có) ===
    if (hasStroke) {
        Color c(strokeA, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB());
        Pen pen(c, strokeWidth);
        pen.SetLineJoin(LineJoinRound);
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        g.DrawLines(&pen, points.data(), static_cast<INT>(points.size()));
    }
}

static std::wstring Utf8ToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], wideLen);

    if (!wstr.empty() && wstr.back() == L'\0')
        wstr.pop_back();

    return wstr;
}

SvgText::SvgText(float x, float y, const std::string& content)
    : x(x), y(y), text(content),
    fontFamily("Arial"), fontSize(16.0f),
    fontWeight("normal"), fontStyle("normal"),
    textAnchor("start") {
}

void SvgText::SetText(const std::string& content) {
    text = content;
}

void SvgText::SetPosition(float x, float y) {
    this->x = x;
    this->y = y;
}

void SvgText::SetFont(const std::string& family, float size) {
    fontFamily = family;
    fontSize = size;
}

void SvgText::SetStyle(const std::string& weight, const std::string& style) {
    fontWeight = weight;
    fontStyle = style;
}

void SvgText::SetAnchor(const std::string& anchor) {
    textAnchor = anchor;
}

void SvgText::DrawImpl(Graphics& g, BYTE fillA, BYTE strokeA) const {
    if (text.empty()) return;

    std::wstring wText = Utf8ToWstring(text);
    std::wstring wFont = Utf8ToWstring(fontFamily);

    FontFamily family(wFont.c_str());
    INT style = FontStyleRegular;
    if (fontStyle == "italic") style |= FontStyleItalic;
    if (fontWeight == "bold") style |= FontStyleBold;

    Font font(&family, fontSize, style, UnitPixel);

    // Đo kích thước chữ để căn chỉnh anchor
    RectF layoutRect;
    g.MeasureString(wText.c_str(), -1, &font, PointF(0, 0), &layoutRect);
    float offsetX = 0.0f;
    if (textAnchor == "middle") offsetX = -layoutRect.Width / 2.0f;
    else if (textAnchor == "end") offsetX = -layoutRect.Width;

    PointF pos(x + offsetX, y);

    // Fill (màu chữ)
    if (hasFill) {
        Color c(fillA, fillColor.GetR(), fillColor.GetG(), fillColor.GetB());
        SolidBrush brush(c);
        g.DrawString(wText.c_str(), -1, &font, pos, &brush);
    }

    // Stroke (viền chữ)
    if (hasStroke) {
        Color c(strokeA, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB());
        Pen pen(c, strokeWidth);
        GraphicsPath path;
        path.AddString(wText.c_str(), -1, &family, style, fontSize, pos, nullptr);
        g.DrawPath(&pen, &path);
    }
}