#include"polyline.h"
#include <algorithm>
#include <cctype>
SVGPOLYLINE::SVGPOLYLINE(const std::vector<PointF>& pts)
    : points(pts) {
}

SVGPOLYLINE::SVGPOLYLINE(float x,float y){
    points.emplace_back(x, y);
}
void SVGPOLYLINE::AddPoint(float x, float y) {
    points.emplace_back(x, y);
}

void SVGPOLYLINE::SetPoints(const std::vector<PointF>& pts) {
    points = pts;
}

const std::vector<PointF>& SVGPOLYLINE::GetPoints() const {
    return points;
}

void SVGPOLYLINE::DrawImpl(Graphics& g, BYTE fillA, BYTE strokeA) const {
    if (points.size() < 2)
        return; // cần ít nhất 2 điểm để vẽ đường

    // === Fill (nếu có) ===
    if (hasFill && points.size() >= 3) {
        Color c(fillA, fillColor.GetR(), fillColor.GetG(), fillColor.GetB());
        SolidBrush brush(c);
        g.FillPolygon(&brush, points.data(), static_cast<INT>(points.size()));
    }

    // === Stroke (nếu có) ===
    if (hasStroke && strokeWidth > 0) {
        Color c(strokeA, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB());
        Pen pen(c, strokeWidth);
        pen.SetMiterLimit(strokeMiterLimit);
        
        // Áp dụng stroke-linejoin
        std::string joinLower = strokeLinejoin;
        for (auto& c : joinLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (joinLower == "round") {
            pen.SetLineJoin(LineJoinRound);
        } else if (joinLower == "bevel") {
            pen.SetLineJoin(LineJoinBevel);
        } else {
            pen.SetLineJoin(LineJoinMiter); // "miter"
        }
        
        // Áp dụng stroke-linecap
        std::string capLower = strokeLinecap;
        for (auto& c : capLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (capLower == "round") {
            pen.SetStartCap(LineCapRound);
            pen.SetEndCap(LineCapRound);
        } else if (capLower == "square") {
            pen.SetStartCap(LineCapSquare);
            pen.SetEndCap(LineCapSquare);
        } else {
            pen.SetStartCap(LineCapFlat); // "butt"
            pen.SetEndCap(LineCapFlat);
        }
        
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