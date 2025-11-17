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
#include <vector>
using namespace std;
class SVGDOCUMENT {
private:
    vector<SVGSHAPE*> shapes;
    float zoomFactor;
	float rotateAngle;
    READSVG read;
public:
    SVGDOCUMENT();
    ~SVGDOCUMENT();
    void AddShape(SVGSHAPE* s);
    void Render(Gdiplus::Graphics& g, int windowWidth, int windowHeight) const;
    BYTE clamp255(int v);
    void ApplyCommonPaint(SVGSHAPE* shp, int i);
    void LoadSvgToDocument(const std::string& path);
    void ZoomIn();
	void ZoomOut();
	void RotateLeft();
	void RotateRight();

};
#endif // !SVGDOCUMENT_H
