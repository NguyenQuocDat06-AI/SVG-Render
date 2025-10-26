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