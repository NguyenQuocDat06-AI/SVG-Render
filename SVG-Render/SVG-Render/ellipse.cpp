#include "ellipse.h"
SVGELLIPSE::SVGELLIPSE() : cx(0), cy(0), rx(0), ry(0) {}
SVGELLIPSE::SVGELLIPSE(float cx, float cy, float rx, float ry)
	: cx(cx), cy(cy), rx(rx), ry(ry) {
}
void SVGELLIPSE::SetCenter(float cx, float cy) {
	this->cx = cx;
	this->cy = cy;
}
void SVGELLIPSE::SetRadii(float rx, float ry) {
	this->rx = rx;
	this->ry = ry;
}
void SVGELLIPSE::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
	// Tạo hình ellipse
	Gdiplus::GraphicsPath path;
	path.AddEllipse(Gdiplus::RectF(cx - rx, cy - ry, 2 * rx, 2 * ry));
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
