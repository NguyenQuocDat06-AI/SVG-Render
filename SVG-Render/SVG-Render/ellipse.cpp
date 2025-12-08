#include "ellipse.h"
#include <algorithm>
#include <cctype>
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
	
	// Áp dụng fill-rule
	std::string ruleLower = fillRule;
	for (auto& c : ruleLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	if (ruleLower == "evenodd") {
		path.SetFillMode(Gdiplus::FillModeAlternate);
	} else {
		path.SetFillMode(Gdiplus::FillModeWinding); // "nonzero"
	}
	
	path.AddEllipse(Gdiplus::RectF(cx - rx, cy - ry, 2 * rx, 2 * ry));
	// Fill
	if (hasFill && finalFillAlpha > 0) {
		Gdiplus::SolidBrush fillBrush(Gdiplus::Color(finalFillAlpha, fillColor.GetR(), fillColor.GetG(), fillColor.GetB()));
		g.FillPath(&fillBrush, &path);
	}
	// Stroke
	if (hasStroke && strokeWidth > 0) {
		Gdiplus::Pen strokePen(Gdiplus::Color(finalStrokeAlpha, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB()), strokeWidth);
		strokePen.SetMiterLimit(strokeMiterLimit);
		
		// Áp dụng stroke-linejoin
		std::string joinLower = strokeLinejoin;
		for (auto& c : joinLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		if (joinLower == "round") {
			strokePen.SetLineJoin(Gdiplus::LineJoinRound);
		} else if (joinLower == "bevel") {
			strokePen.SetLineJoin(Gdiplus::LineJoinBevel);
		} else {
			strokePen.SetLineJoin(Gdiplus::LineJoinMiter); // "miter"
		}
		
		g.DrawPath(&strokePen, &path);
	}
}
