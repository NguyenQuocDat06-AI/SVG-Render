#ifndef CIRCLE_H
#define CIRCLE_H
#include "svgshape.h"
class SVGCIRCLE : public SVGSHAPE {
private:
	float cx;     // Tọa độ tâm circle
	float cy;
	float r;      // Bán kính
public:
	SVGCIRCLE();
	SVGCIRCLE(float cx, float cy, float r);
	void SetCenter(float cx, float cy);
	void SetRadius(float r);
	virtual void DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const override;
};
#endif // !CIRCLE_H
