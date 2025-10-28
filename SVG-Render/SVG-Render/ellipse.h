#ifndef ELLIPSE_H
#define ELLIPSE_H
#include "svgshape.h"
class SVGELLIPSE : public SVGSHAPE {
private:
	float cx;     // Tọa độ tâm ellipse
	float cy;
	float rx;     // Bán kính theo trục x
	float ry;     // Bán kính theo trục y
public:
	SVGELLIPSE();
	SVGELLIPSE(float cx, float cy, float rx, float ry);
	void SetCenter(float cx, float cy);
	void SetRadii(float rx, float ry);
	virtual void DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const override;
};

#endif // !ELLIPSE_H