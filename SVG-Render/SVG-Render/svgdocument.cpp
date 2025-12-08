#include "svgdocument.h"
#include <tuple>     
SVGDOCUMENT::~SVGDOCUMENT() {
    for (auto s : shapes) delete s;
}
void SVGDOCUMENT::AddShape(SVGSHAPE* s) {
    shapes.push_back(s);
}

void SVGDOCUMENT::Render(Gdiplus::Graphics& g, int destW, int destH) const {
    // 1. Kiểm tra xem file có viewBox không (thường nằm ở thẻ gốc - node 0)
    if (read.HasViewBox(0)) {
       
        auto vb = read.GetViewBox(0);
        float vx = std::get<0>(vb); // x
        float vy = std::get<1>(vb); // y
        float vw = std::get<2>(vb); // width
        float vh = std::get<3>(vb); // height

        if (vw > 0 && vh > 0 && destW > 0 && destH > 0) {
            // --- TÍNH TOÁN TỶ LỆ SCALE ---
            float scaleX = (float)destW / vw;
            float scaleY = (float)destH / vh;

            // Lấy tỷ lệ nhỏ hơn để hình nằm trọn trong màn hình ("meet")
            // Nếu muốn hình phủ kín ("slice"), dùng std::max
            float scale = min(scaleX, scaleY);

            // --- TÍNH TOÁN VỊ TRÍ CĂN GIỮA (CENTER) ---
            // Phần dư ra của màn hình chia đôi
            float tx = (destW - vw * scale) / 2.0f;
            float ty = (destH - vh * scale) / 2.0f;

            // --- ÁP DỤNG TRANSFROM VÀO GRAPHICS ---
            // Thứ tự quan trọng:
            // 1. Dịch chuyển vùng vẽ ra giữa màn hình
            g.TranslateTransform(tx, ty);

            // 2. Phóng to/thu nhỏ theo tỷ lệ đã tính
            g.ScaleTransform(scale, scale);

            // 3. Dịch chuyển ngược lại để gốc tọa độ khớp với (vx, vy) của viewBox
            g.TranslateTransform(-vx, -vy);
        }
    }

    // 2. Vẽ các hình như bình thường
    // Lúc này Graphics 'g' đã mang ma trận biến đổi, nên các tọa độ nhỏ trong SVG
    // sẽ tự động được phóng to lên màn hình.
    for (auto s : shapes) {
        s->Draw(g);
    }

    // Reset lại transform để không ảnh hưởng nếu vẽ cái khác sau đó
    g.ResetTransform();
}
BYTE SVGDOCUMENT::clamp255(int v) {
    if (v < 0) v = 0; else if (v > 255) v = 255;
    return (BYTE)v;
}
// Helper function để áp dụng transform tích lũy từ cha và transform của chính shape
// Sử dụng các phương thức GDI++ (Translate, Scale, RotateAt) thay vì nhân ma trận
void SVGDOCUMENT::ApplyTransformToShape(SVGSHAPE* shp, int index,float originX , float originY) {
    Gdiplus::Matrix finalMatrix; 

    auto accumOps = read.GetAccumulatedTransformOperations(index);
    auto localOps = read.GetTransformOperations(index);

    std::vector<TransformOperation> allOps;
    allOps.clear();
    allOps.insert(allOps.end(), accumOps.begin(), accumOps.end()); // cha trước
    allOps.insert(allOps.end(), localOps.begin(), localOps.end()); // con sau

    for (const auto& op : allOps) {
        if (std::isnan(op.values[0]) || std::isnan(op.values[1])) continue;

        switch (op.type) {
        case TransformType::Translate:
            finalMatrix.Translate(op.values[0], op.values[1], Gdiplus::MatrixOrderPrepend);
            break;
        case TransformType::Scale:
            finalMatrix.Scale(op.values[0], op.values[1], Gdiplus::MatrixOrderPrepend);
            break;
        case TransformType::Rotate:
            if (originX != 0.0f || originY != 0.0f) {
                Gdiplus::PointF center(originX, originY);
                finalMatrix.RotateAt(op.values[0], center, Gdiplus::MatrixOrderPrepend);
            }
            else {
                finalMatrix.Rotate(op.values[0], Gdiplus::MatrixOrderPrepend);
            }
            break;
        case TransformType::RotateAt:
        {
            Gdiplus::PointF center(op.values[1], op.values[2]);
            finalMatrix.RotateAt(op.values[0], center, Gdiplus::MatrixOrderPrepend);
        }
        break;
        //case TransformType::Matrix:
        //{
        //    Gdiplus::Matrix m(op.values[0], op.values[1], op.values[2],
        //        op.values[3], op.values[4], op.values[5]);
        //    // Kiểm tra ma trận con có tạo được không
        //    if (m.GetLastStatus() == Gdiplus::Ok) {
        //        finalMatrix.Multiply(&m, Gdiplus::MatrixOrderAppend);
        //    }
        //}
        //break;
        //case TransformType::SkewX:
        //case TransformType::SkewY:
        //{
        //    // Đảm bảo góc không gây ra tan() vô cực (vd: 90 độ)
        //    float rad = op.values[0] * 3.14159265358979323846f / 180.f;
        //    float tanVal = std::tan(rad);
        //    Gdiplus::Matrix mtx;
        //    if (op.type == TransformType::SkewX)
        //        mtx.SetElements(1.0f, 0.0f, tanVal, 1.0f, 0.0f, 0.0f);
        //    else
        //        mtx.SetElements(1.0f, tanVal, 0.0f, 1.0f, 0.0f, 0.0f);

        //    finalMatrix.Multiply(&mtx, Gdiplus::MatrixOrderAppend);
        //}
        //break;
        }
    }

    if (finalMatrix.GetLastStatus() == Gdiplus::Ok) {
        shp->SetTransform(finalMatrix);
    }
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
        // Nếu không có stroke trên node này, thử kế thừa từ <g> cha
        if (stroke.empty()) {
            std::string inheritedStroke = read.GetInheritedAttribute(i, "stroke");
            if (!inheritedStroke.empty()) {
                stroke = read.ParseColorPublic(inheritedStroke);
            }
        }
        float sw = read.GetStrokeWidth(i);
        // Nếu không có stroke-width, thử kế thừa
        if (sw <= 0.f) {
            std::string inheritedSW = read.GetInheritedAttribute(i, "stroke-width");
            if (!inheritedSW.empty()) {
                sw = read.ParseFloatPublic(inheritedSW, 1.0f);
            }
        }

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

    // ===== stroke miterlimit & fill-rule & stroke linecap/linejoin =====
    shp->SetStrokeMiterLimit(read.GetStrokeMiterlimit(i));
    shp->SetFillRule(read.GetFillRule(i));
    shp->SetStrokeLinecap(read.GetStrokeLinecap(i));
    shp->SetStrokeLinejoin(read.GetStrokeLinejoin(i));
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
            ApplyTransformToShape(s, i,x,y);
            AddShape(s);
        }
        else if (tag == "circle") {
            float cx = read.GetCx(i), cy = read.GetCy(i), r = read.GetR(i);
            auto* s = new SVGCIRCLE(cx, cy, r);
            ApplyCommonPaint(s, i);
            ApplyTransformToShape(s, i);
            AddShape(s);
        }
        else if (tag == "ellipse") {
            float cx = read.GetCx(i), cy = read.GetCy(i), rx = read.GetRx(i), ry = read.GetRy(i);
            auto* s = new SVGELLIPSE(cx, cy, rx, ry);
            ApplyCommonPaint(s, i);
            ApplyTransformToShape(s, i);
            AddShape(s);
        }
        else if (tag == "line") {
            float x1 = read.GetX1(i), y1 = read.GetY1(i), x2 = read.GetX2(i), y2 = read.GetY2(i);
            auto* s = new SVGLINE(x1, y1, x2, y2);
            ApplyCommonPaint(s, i);
            ApplyTransformToShape(s, i);
            AddShape(s);
        }
        else if (tag == "polyline") {
            auto pts = read.GetPoints(i);
            auto* s = new SVGPOLYLINE();
            for (auto& p : pts) s->AddPoint(p.first, p.second);
            ApplyCommonPaint(s, i);
            ApplyTransformToShape(s, i);
            AddShape(s);
        }
        else if (tag == "polygon") {
            auto pts = read.GetPoints(i);
            auto* s = new SVGPOLYGON();
            for (auto& p : pts) s->AddPoint(p.first, p.second);
            ApplyCommonPaint(s, i);
            ApplyTransformToShape(s, i);
            AddShape(s);
        }
        else if (tag == "text") {
            // 1. Lấy tọa độ cơ bản (nếu không có trong thẻ thì mặc định là 0,0)
            float x = read.GetX(i);
            float y = read.GetY(i);

            float dx = read.GetDx(i);
            float dy = read.GetDy(i);
            // Lấy nội dung text
            std::string content = read.GetNode(i)._text;

            // Debug: Nếu content rỗng, thử tìm lại trong node con (đề phòng lỗi parser)
            if (content.empty()) {
                // Logic này thường nằm ở ReadSVG, nhưng kiểm tra nhanh ở đây
                // Nếu content vẫn rỗng thì text sẽ không hiện.
            }

            auto* s = new SVGTEXT(x, y,dx,dy, content);

            // ---------------------------------------------------------
            // FIX 1: Xử lý Font Family (Bạn đã có, nhưng giữ lại để đồng bộ)
            // ---------------------------------------------------------
            std::string fontList = read.GetFontFamily(i);
            std::string primaryFont = "Arial";
            if (!fontList.empty()) {
                size_t commaPos = fontList.find(',');
                if (commaPos != std::string::npos) {
                    primaryFont = fontList.substr(0, commaPos);
                }
                else {
                    primaryFont = fontList;
                }
            }

            // Xử lý Font Size & Style
            float fontSize = read.GetFontSize(i);
            if (fontSize <= 0.f) {
                std::string inheritedFS = read.GetInheritedAttribute(i, "font-size");
                if (!inheritedFS.empty()) {
                    fontSize = read.ParseFloatPublic(inheritedFS, 16.0f);
                }
            }
            s->SetFont(primaryFont, fontSize);
            s->SetStyle(read.GetFontWeight(i), read.GetFontStyle(i));
            s->SetAnchor(read.GetTextAnchor(i));

            // Paint
            ApplyCommonPaint(s, i);

            // Áp dụng transform (từ cha và của chính text)
            ApplyTransformToShape(s, i,x,y);

            AddShape(s);
        }
        else if (tag == "path") {  // path có thể dùng linearGradient
            std::string d = read.GetPathD(i);
            auto* s = new SVGPATH(d);

            // Kiểm tra fill="url(#id)" để gán gradient nếu có
            std::string rawFill = read.GetAttributeRaw(i, "fill", "");
            if (!rawFill.empty()) {
                // dạng url(#fill0)
                auto pos = rawFill.find("url(");
                if (pos != std::string::npos) {
                    auto hashPos = rawFill.find('#', pos);
                    auto endPos = rawFill.find(')', pos);
                    if (hashPos != std::string::npos && endPos != std::string::npos && hashPos < endPos) {
                        std::string id = rawFill.substr(hashPos + 1, endPos - hashPos - 1);
                        LinearGradientDef grd;
                        if (read.TryGetLinearGradient(id, grd) && !grd.colors.empty()) {
                            std::vector<Gdiplus::Color> cols;
                            std::vector<float> offs;
                            for (size_t k = 0; k < grd.colors.size(); ++k) {
                                const auto& c = grd.colors[k];
                                if (c.size() >= 3) {
                                    cols.emplace_back(255, (BYTE)c[0], (BYTE)c[1], (BYTE)c[2]);
                                    float off = (k < grd.offsets.size()) ? grd.offsets[k] : 0.0f;
                                    offs.push_back(off);
                                }
                            }
                            if (!cols.empty()) {
                                Gdiplus::PointF p1(grd.x1, grd.y1);
                                Gdiplus::PointF p2(grd.x2, grd.y2);
                                s->SetLinearGradient(p1, p2, cols, offs);
                            }
                        }
                    }
                }
            }
            ApplyCommonPaint(s, i);
            ApplyTransformToShape(s, i);
            AddShape(s);
        }
        // các tag khác (defs, g, use, image, ...) tùy bạn bổ sung dần
    }
}