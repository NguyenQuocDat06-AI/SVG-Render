#include"line.h"
#include <algorithm>
#include <cctype>
SVGLINE::SVGLINE(float x1, float y1, float x2, float y2)
    : x1(x1), y1(y1), x2(x2), y2(y2) {
}

// Cập nhật lại 2 điểm đầu – cuối
void SVGLINE::SetPoints(float x1, float y1, float x2, float y2) {
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
}

// Hàm vẽ đường thẳng (override từ SvgShape)
void SVGLINE::DrawImpl(Graphics& g, BYTE fillA, BYTE strokeA) const {
    if (!hasStroke || strokeWidth <= 0) return;
    
    Pen pen(Color(strokeA, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB()), strokeWidth);
    
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
    
    g.DrawLine(&pen, x1, y1, x2, y2);
}
