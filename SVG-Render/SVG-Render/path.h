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
    Gdiplus::Matrix gradMatrix;
    Gdiplus::PointF gStart{ 0,0 };
    Gdiplus::PointF gEnd{ 0,0 };
    std::vector<Gdiplus::Color> gColors;
    std::vector<float> gOffsets;

    bool hasGradTransform = false;
    bool useRadialGradient = false;
    float radCx = 0.5f, radCy = 0.5f, radR = 0.5f;
    float radFx = 0.5f, radFy = 0.5f;
    bool radUserSpace = false; // true nếu userSpaceOnUse
    std::vector<Gdiplus::Color> radColors;
    std::vector<float> radOffsets;
public:
    explicit SVGPATH(string d);
    void SetLinearGradient(const Gdiplus::PointF& p1, const Gdiplus::PointF& p2,
        const std::vector<Gdiplus::Color>& colors,
        const std::vector<float>& offsets,
        const std::vector<float>& transformMtx); // <--- Thêm tham số này

    // Cập nhật tham số cho SetRadialGradient
    void SetRadialGradient(float cx, float cy, float r, float fx, float fy,
        bool userSpace,
        const std::vector<Gdiplus::Color>& colors,
        const std::vector<float>& offsets,
        const std::vector<float>& transformMtx); // <--- Thêm tham số này
protected:
    void DrawImpl(Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const override;
};

#endif // !PATH_H