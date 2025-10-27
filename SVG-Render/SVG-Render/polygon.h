#ifndef POLYGON_H
#define POLYGON_H
#include <vector>
#include "svgshape.h"
class SVGPOLYGON : public SVGSHAPE {
private:
	std::vector<Gdiplus::PointF> points;
public:
	SVGPOLYGON();
	void AddPoint(float x, float y);
	void SetPoints(const std::vector<Gdiplus::PointF>& pts);
	virtual void DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const override;
};

#endif // !POLYGON_H
