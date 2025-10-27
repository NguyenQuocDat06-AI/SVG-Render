#include "rect.h"

SVGRECT::SVGRECT() : x(0), y(0), width(0), height(0), rx(0), ry(0) {}

SVGRECT::SVGRECT(float x, float y, float width, float height, float rx, float ry)
	: x(x), y(y), width(width), height(height), rx(rx), ry(ry) {
}

void SVGRECT::SetPosition(float x, float y) {
	this->x = x;
	this->y = y;
}

void SVGRECT::SetSize(float width, float height) {
	this->width = width;
	this->height = height;
}

void SVGRECT::SetCornerRadius(float rx, float ry) {
	this->rx = rx;
	this->ry = ry;
}

void SVGRECT::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
	// Tạo hình chữ nhật với bo góc nếu cần

	Gdiplus::GraphicsPath path;

	if (rx > 0 || ry > 0) {
		float cornerRx = rx > 0 ? rx : ry;
		float cornerRy = ry > 0 ? ry : rx;
		path.AddArc(x, y, 2 * cornerRx, 2 * cornerRy, 180, 90); // Top-left
		path.AddArc(x + width - 2 * cornerRx, y, 2 * cornerRx, 2 * cornerRy, 270, 90); // Top-right
		path.AddArc(x + width - 2 * cornerRx, y + height - 2 * cornerRy, 2 * cornerRx, 2 * cornerRy, 0, 90); // Bottom-right
		path.AddArc(x, y + height - 2 * cornerRy, 2 * cornerRx, 2 * cornerRy, 90, 90); // Bottom-left
		path.CloseFigure();
	} else {
		path.AddRectangle(Gdiplus::RectF(x, y, width, height));
	}
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