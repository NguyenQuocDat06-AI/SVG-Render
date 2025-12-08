#ifndef PATH_H
#define PATH_H

#include <iostream>
#include <string>
#include "svgshape.h"
using namespace std;
class SVGPATH : public SVGSHAPE {
private:
    string d;
    // Thông tin gradient tuyến tính (nếu có)
    bool useLinearGradient = false;
    Gdiplus::PointF gStart{ 0,0 };
    Gdiplus::PointF gEnd{ 0,0 };
    std::vector<Gdiplus::Color> gColors;
    std::vector<float> gOffsets;
public:
    explicit SVGPATH(string d);

    void SetLinearGradient(const Gdiplus::PointF& p1, const Gdiplus::PointF& p2,
        const std::vector<Gdiplus::Color>& colors,
        const std::vector<float>& offsets);
protected:
    void DrawImpl(Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const override;
};

#endif // !PATH_H