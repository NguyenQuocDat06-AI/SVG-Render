#ifndef LINE_H
#define LINE_H
#include <vector>
#include "svgshape.h"
class SVGLINE : public SVGSHAPE {
private:
    float x1, y1, x2, y2;

protected:
    void DrawImpl(Graphics& g, BYTE fillA, BYTE strokeA) const override;

public:
    SVGLINE(float x1, float y1, float x2, float y2);
    void SetPoints(float x1, float y1, float x2, float y2);
};

#endif // !LINE_H


