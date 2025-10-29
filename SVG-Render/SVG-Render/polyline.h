#ifndef POLYLINE_H
#define POLYLINE_H
#include <vector>
#include "svgshape.h"
class SVGPOLYLINE : public SVGSHAPE {
protected:
    std::vector<Gdiplus::PointF> points;

public:
    SVGPOLYLINE() = default;
    SVGPOLYLINE(const std::vector<Gdiplus::PointF>& pts);
    SVGPOLYLINE(float x, float y);

    void AddPoint(float x, float y);
    void SetPoints(const std::vector<Gdiplus::PointF>& pts);
    const std::vector<Gdiplus::PointF>& GetPoints() const;

protected:
    void DrawImpl(Gdiplus::Graphics& g, BYTE fillA, BYTE strokeA) const override;
};

#endif // !POLYLINE_H

