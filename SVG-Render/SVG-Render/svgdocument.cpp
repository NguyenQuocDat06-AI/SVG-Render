#include "svgdocument.h"
SVGDOCUMENT::~SVGDOCUMENT() {
    for (auto s : shapes) delete s;
}
void SVGDOCUMENT::AddShape(SVGSHAPE* s) {
    shapes.push_back(s);
}

void SVGDOCUMENT::Render(Gdiplus::Graphics& g) const {
    for (auto s : shapes) {
        s->Draw(g);
    }
}