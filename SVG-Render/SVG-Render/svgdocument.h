#ifndef SVGDOCUMENT_H
#define SVGDOCUMENT_H
#include "SVGSHAPE.h"
#include <vector>
using namespace std;
class SVGDOCUMENT {
private:
    vector<SVGSHAPE*> shapes;
public:
    ~SVGDOCUMENT();
    void AddShape(SVGSHAPE* s);
    void Render(Gdiplus::Graphics& g) const;
};
#endif // !SVGDOCUMENT_H
