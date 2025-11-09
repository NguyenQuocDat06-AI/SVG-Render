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
    READSVG read;
public:
    ~SVGDOCUMENT();
    void AddShape(SVGSHAPE* s);
    void Render(Gdiplus::Graphics& g) const;
    BYTE clamp255(int v);
    void ApplyCommonPaint(SVGSHAPE* shp, int i);
    void LoadSvgToDocument(const std::string& path);
};
#endif // !SVGDOCUMENT_H
