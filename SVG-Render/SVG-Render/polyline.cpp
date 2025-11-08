#include"polyline.h"
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