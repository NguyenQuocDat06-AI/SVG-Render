#include "circle.h"

SVGCIRCLE::SVGCIRCLE()
    : SVGELLIPSE(0.0f, 0.0f, 0.0f, 0.0f) {
}

SVGCIRCLE::SVGCIRCLE(float cx, float cy, float r)
    : SVGELLIPSE(cx, cy, r, r) {
}

void SVGCIRCLE::SetRadius(float r) {
    // dùng API của ellipse để set cả rx và ry
    SVGELLIPSE::SetRadii(r, r);
}
