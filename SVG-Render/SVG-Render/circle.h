#ifndef CIRCLE_H
#define CIRCLE_H
#include "ellipse.h"

class SVGCIRCLE : public SVGELLIPSE {
public:
    SVGCIRCLE();                                // (0,0, r=0)
    SVGCIRCLE(float cx, float cy, float r);     // khởi tạo ellipse với rx=ry=r

    // Đặt bán kính: duy trì rx=ry=r
    void SetRadius(float r);

    // Kế thừa toàn bộ phần còn lại từ SVGELLIPSE (fill/stroke/opacity/Draw)

private:
    // Ẩn SetRadii để không phá bất biến hình tròn (rx phải = ry)
    using SVGELLIPSE::SetRadii;
};

#endif // !CIRCLE_H
