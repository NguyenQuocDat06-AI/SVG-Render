#ifndef SVGDOCUMENT_H
#define SVGDOCUMENT_H
#include "SVGSHAPE.h"
#include "readsvg.h"
#include "rect.h"
#include "polygon.h"
#include "ellipse.h"
#include "circle.h"
#include "text.h"
#include "line.h"
#include "polyline.h"
#include "path.h"
#include <algorithm>
#include <vector>
using namespace std;
class SVGDOCUMENT {
private:
    vector<SVGSHAPE*> shapes;
    float zoomFactor;
	float rotationAngle; // góc xoay (độ)
	float translateX;
	float translateY;
    READSVG read;
    // Helper function để áp dụng transform tích lũy từ cha và transform của chính shape
    void ApplyTransformToShape(SVGSHAPE* shp, int index,float originX = 0.0f, float originY = 0.0f);
public:
    SVGDOCUMENT();
    ~SVGDOCUMENT();
    void AddShape(SVGSHAPE* s);
    void Render(Gdiplus::Graphics& g, int destW, int destH) const;
    BYTE clamp255(int v);
    void ApplyCommonPaint(SVGSHAPE* shp, int i);
    void LoadSvgToDocument(const std::string& path);
    void ZoomIn();
    void ZoomOut();
    void RotateLeft();
    void RotateRight();
    void MoveUp();
    void MoveDown();
    void MoveLeft();
    void MoveRight();
};
#endif // !SVGDOCUMENT_H
