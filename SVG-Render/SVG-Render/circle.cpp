#include "circle.h"
SVGCIRCLE::SVGCIRCLE() : cx(0), cy(0), r(0) {}
SVGCIRCLE::SVGCIRCLE(float cx, float cy, float r)
	: cx(cx), cy(cy), r(r) {
}
void SVGCIRCLE::SetCenter(float cx, float cy) {
	this->cx = cx;
	this->cy = cy;
}
void SVGCIRCLE::SetRadius(float r) {
	this->r = r;
}
void SVGCIRCLE::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
	// Tạo hình circle
	Gdiplus::GraphicsPath path;
	path.AddEllipse(Gdiplus::RectF(cx - r, cy - r, 2 * r, 2 * r));
	// Fill
	if (hasFill) {
		Gdiplus::SolidBrush fillBrush(Gdiplus::Color(finalFillAlpha, fillColor.GetR(), fillColor.GetG(), fillColor.GetB()));
		g.FillPath(&fillBrush, &path);
	}
	// Stroke
	if (hasStroke) {
		Gdiplus::Pen strokePen(Gdiplus::Color(finalStrokeAlpha, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB()), strokeWidth);
		g.DrawPath(&strokePen, &path);
	}
}