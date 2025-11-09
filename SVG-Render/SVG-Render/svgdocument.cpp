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
BYTE SVGDOCUMENT::clamp255(int v) {
    if (v < 0) v = 0; else if (v > 255) v = 255;
    return (BYTE)v;
}
void SVGDOCUMENT::ApplyCommonPaint(SVGSHAPE* shp, int i) {
    // display / visibility / opacity
    if (read.GetDisplayNone(i)) shp->SetDisplayNone(true);
    shp->SetVisible(read.GetVisible(i));
    shp->SetOverallOpacity(read.GetOpacity(i));

    const std::string tag = read.GetTagName(i);

    // ===== FILL =====
    {
        auto fill = read.GetFill(i);     // rỗng nếu không có hoặc 'none' (tùy cách bạn parse)
        if (!fill.empty()) {
            shp->EnableFill(clamp255(fill[0]), clamp255(fill[1]), clamp255(fill[2]));
            shp->SetFillOpacity(read.GetFillOpacity(i));
        }
        else {
            // Nếu không có fill nhưng cũng không phải fill="none" → áp mặc định theo SVG
            if (!read.IsFillNone(i)) {
                // SVG default fill = black cho mọi shape có diện tích (rect/circle/ellipse/polygon/polyline/path/text)
                // (line không có diện tích nên fill không có tác dụng, nhưng áp cũng không gây lỗi)
                shp->EnableFill(0, 0, 0);
                shp->SetFillOpacity(read.GetFillOpacity(i)); // dùng fill-opacity nếu có (vd: 0.5)
            }
            else {
                shp->DisableFill();
            }
        }
    }

    // ===== STROKE ===== (SVG default stroke = none)
    {
        auto stroke = read.GetStroke(i);
        float sw = read.GetStrokeWidth(i);

        // Với <text>: chỉ stroke khi thực có stroke
        if (tag == "text" && stroke.empty()) {
            shp->DisableStroke();
        }
        else if (!stroke.empty() && sw > 0.f) {
            shp->EnableStroke(clamp255(stroke[0]), clamp255(stroke[1]), clamp255(stroke[2]), sw);
            shp->SetStrokeOpacity(read.GetStrokeOpacity(i));
        }
        else {
            shp->DisableStroke();
        }
    }
}
void SVGDOCUMENT::LoadSvgToDocument(const std::string& path) {
    read.ParseFromBuffer(path);

    const int n = read.GetNodeCount();
    for (int i = 0; i < n; ++i) {
        const std::string tag = read.GetTagName(i);

        if (tag == "rect") {
            float x = read.GetRectX(i);
            float y = read.GetRectY(i);
            float w = read.GetRectWidth(i);
            float h = read.GetRectHeight(i);
            float rx = read.GetRectRx(i);
            float ry = read.GetRectRy(i);

            auto* s = new SVGRECT(x, y, w, h, rx, ry);
            ApplyCommonPaint(s, i);
            AddShape(s);
        }
        else if (tag == "circle") {
            float cx = read.GetCx(i), cy = read.GetCy(i), r = read.GetR(i);
            auto* s = new SVGCIRCLE(cx, cy, r);
            ApplyCommonPaint(s, i);
            AddShape(s);
        }
        else if (tag == "ellipse") {
            float cx = read.GetCx(i), cy = read.GetCy(i), rx = read.GetRx(i), ry = read.GetRy(i);
            auto* s = new SVGELLIPSE(cx, cy, rx, ry);
            ApplyCommonPaint(s, i);
            AddShape(s);
        }
        else if (tag == "line") {
            float x1 = read.GetX1(i), y1 = read.GetY1(i), x2 = read.GetX2(i), y2 = read.GetY2(i);
            auto* s = new SVGLINE(x1, y1, x2, y2);
            ApplyCommonPaint(s, i);
            AddShape(s);
        }
        else if (tag == "polyline") {
            auto pts = read.GetPoints(i);
            auto* s = new SVGPOLYLINE();
            for (auto& p : pts) s->AddPoint(p.first, p.second);
            ApplyCommonPaint(s, i);
            AddShape(s);
        }
        else if (tag == "polygon") {
            auto pts = read.GetPoints(i);
            auto* s = new SVGPOLYGON();
            for (auto& p : pts) s->AddPoint(p.first, p.second);
            ApplyCommonPaint(s, i);
            AddShape(s);
        }
        else if (tag == "text") {
            float x = read.GetX(i), y = read.GetY(i);
            std::string content = read.GetNode(i)._text;               // nội dung giữa thẻ
            auto* s = new SVGTEXT(x, y, content);

            // font/layout
            s->SetFont(read.GetFontFamily(i), read.GetFontSize(i));
            s->SetStyle(read.GetFontWeight(i), read.GetFontStyle(i));
            s->SetAnchor(read.GetTextAnchor(i));

            // paint
            ApplyCommonPaint(s, i);
            AddShape(s);
        }
        // else if (tag == "path") {  // TODO: cần parser path 'd'
        //     std::string d = read.GetPathD(i);
        //     auto* s = new SVGPATH(d); // nếu bạn có lớp này
        //     ApplyCommonPaint(s, R, i);
        //     AddShape(s);
        // }
        // các tag khác (defs, g, use, image, ...) tùy bạn bổ sung dần
    }
}