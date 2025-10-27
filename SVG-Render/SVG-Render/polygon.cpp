#include "polygon.h"

SVGPOLYGON::SVGPOLYGON() : points() {}
void SVGPOLYGON::AddPoint(float x, float y) {
	points.push_back(Gdiplus::PointF(x, y));
}
void SVGPOLYGON::SetPoints(const std::vector<Gdiplus::PointF>& pts) {
	points = pts;
}

void SVGPOLYGON::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
	if (points.size() < 3) {
		// Không đủ điểm để tạo đa giác
		return;
	}
	// Tạo path từ các điểm
	Gdiplus::GraphicsPath path;
	path.AddPolygon(points.data(), static_cast<INT>(points.size()));
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