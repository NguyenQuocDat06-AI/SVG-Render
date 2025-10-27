#ifndef RECT_H
#define RECT_H
#include "svgshape.h"
class SVGRECT : public SVGSHAPE {
private:
	float x;
	float y;
	float width;
	float height;
	float rx; // bán kính bo góc x
	float ry; // bán kính bo góc y
public:
	SVGRECT();
	SVGRECT(float x, float y, float width, float height, float rx = 0.0f, float ry = 0.0f);
	void SetPosition(float x, float y);
	void SetSize(float width, float height);
	void SetCornerRadius(float rx, float ry);
	virtual void DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const override;
};
#endif // !RECT_H
