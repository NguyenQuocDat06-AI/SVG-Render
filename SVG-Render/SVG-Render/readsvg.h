#ifndef READSVG_H
#define READSVG_H
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include "rapidxml.hpp"

#include <windows.h>
#include <gdiplus.h>
#pragma comment (lib, "gdiplus.lib")

using namespace rapidxml;
using namespace std;

struct ATTRIBUTE {
    std::string _tags = "";             // tên thẻ: svg, g, rect, circle, text, line, polyline, polygon, path, ellipse, defs, use, ...
    std::string _text = "";             // text content (nội dung text giữa cặp thẻ) – với <text>, <tspan>
    std::map<std::string, std::string> _att; // map thuộc tính (đã giữ nguyên dạng chuỗi)
    int _parentIndex = -1;              // index của node cha trong _node (để track cấu trúc cây)
    int _depth = 0;                     // độ sâu trong cây (0 = root)
};


struct LinearGradientDef {
    std::string id;
    std::string href; // <--- THÊM BIẾN NÀY
    float x1 = 0.0f, y1 = 0.0f, x2 = 1.0f, y2 = 0.0f;
    std::string units = "objectBoundingBox";
    std::string spreadMethod = "pad";
    std::vector<float> offsets;
    std::vector<std::vector<int>> colors;

    std::vector<float> gradientTransform = { 1, 0, 0, 1, 0, 0 };
};

struct RadialGradientDef {
    std::string id;
    std::string href; // <--- THÊM BIẾN NÀY
    float cx = 0.5f, cy = 0.5f, r = 0.5f;
    float fx = 0.5f, fy = 0.5f;
    std::string units = "objectBoundingBox";
    std::string spreadMethod = "pad";
    std::vector<float> offsets;
    std::vector<std::vector<int>> colors;

    std::vector<float> gradientTransform = { 1, 0, 0, 1, 0, 0 };
};

// Mô tả một transform operation
enum class TransformType {
    Translate,
    Scale,
    Rotate,      // rotate(angle) - xoay quanh gốc
    RotateAt,    // rotate(angle, cx, cy) - xoay quanh điểm
    Matrix,      // matrix(a,b,c,d,e,f)
    SkewX,
    SkewY
};

struct TransformOperation {
    TransformType type;
    float values[6];  // chứa các giá trị tùy theo type
    // Translate: values[0]=tx, values[1]=ty
    // Scale: values[0]=sx, values[1]=sy
    // Rotate: values[0]=angle (độ)
    // RotateAt: values[0]=angle, values[1]=cx, values[2]=cy
    // Matrix: values[0-5]=a,b,c,d,e,f
    // SkewX: values[0]=angle
    // SkewY: values[0]=angle
};

class READSVG {
private:
    std::vector<ATTRIBUTE> _node;
    std::map<std::string, LinearGradientDef> _linearGradients; // id -> gradient
    std::map<std::string, RadialGradientDef> _radialGradients;
    // Duyệt đệ quy toàn bộ cây node (svg, g, path, rect, defs, ...)
    void TraverseNode(xml_node<>* node, int parentIndex, int depth);
    void ResolveGradientStops(const std::string& hrefId, std::vector<float>& outOffsets, std::vector<std::vector<int>>& outColors, int depth) const;
    // ========= Helpers private (khuyến nghị) =========
    std::vector<char> ReadFileToBuffer(const std::string& _path);
    static float ParseFloat(const std::string& s, float def = 0.0f);
    static std::vector<int> ParseColor(const std::string& s);                 // hỗ trợ #rgb/#rrggbb/rgb()/named-color
    static float ParseOpacity(const std::string& s, float def = 1.0f);        // clamp 0..1
    static std::vector<float> ParseFloatList(const std::string& s);           // dùng cho points, dasharray
    static std::vector<TransformOperation> ParseTransformOperations(const std::string& s); // parse thành danh sách operations
    static std::pair<float, float> ParsePair(const std::string& sx, const std::string& sy, float defx = 0, float defy = 0);

public:
    READSVG();
    ~READSVG();

    // ========= Core parse =========
    void ParseFromBuffer(const std::string& _path); // Đọc file và đổ toàn bộ node vào _node
    //void PrintNode();

    // ========= Truy cập node & thuộc tính thô =========
    int  GetNodeCount() const { return (int)_node.size(); }
    ATTRIBUTE GetNode(int index);                     // trả bản sao node (tags, text, att)
    std::string GetTagName(int index);
    std::map<std::string, std::string> GetAttributes(int index);
    std::string GetAttributeRaw(int index, const std::string& key, const std::string& def = "");

    // ========= Thuộc tính chung (presentation) =========
    std::vector<int> getColor(const std::string& str); // giữ API cũ

    // fill / stroke cơ bản
    std::vector<int> GetFill(int index);               // rgb (0..255), nếu "none" trả rỗng
    float            GetFillOpacity(int index);        // 0..1
    std::vector<int> GetStroke(int index);             // rgb (0..255), nếu "none" trả rỗng
    float            GetStrokeOpacity(int index);      // 0..1
    float            GetStrokeWidth(int index);

    // fill / stroke nâng cao
    std::string      GetFillRule(int index);           // "nonzero" | "evenodd"
    std::string      GetStrokeLinecap(int index);      // "butt" | "round" | "square"
    std::string      GetStrokeLinejoin(int index);     // "miter" | "round" | "bevel"
    float            GetStrokeMiterlimit(int index);   // mặc định 4
    std::vector<float> GetStrokeDasharray(int index);  // rỗng nếu "none"
    float            GetStrokeDashoffset(int index);

    // visibility / display / opacity tổng
    bool             GetDisplayNone(int index);        // display="none"
    bool             GetVisible(int index);            // visibility: visible/hidden/collapse
    float            GetOpacity(int index);            // opacity 0..1 (áp toàn shape)

    // transform (ma trận 2D)
    bool             HasTransform(int index);
    // trả về [a,b,c,d,e,f] theo chuẩn SVG transform matrix(a b c d e f)
    // trả về danh sách các transform operations
    std::vector<TransformOperation> GetTransformOperations(int index);

    // id / class / style (inline CSS)
    std::string      GetId(int index);
    std::string      GetClass(int index);
    std::string      GetStyle(int index);              // nguyên chuỗi style="..."

    // ========= Canvas-level =========
    float            GetWidth(int index);              // giữ API cũ
    float            GetHeight(int index);             // giữ API cũ
    // viewBox="minX minY width height"
    bool             HasViewBox(int index) const ;
    std::tuple<float, float, float, float> GetViewBox(int index) const;
    std::string      GetPreserveAspectRatio(int index); // vd: "xMidYMid meet"

    // ========= Hình học chung =========
    float GetX(int index);
    float GetY(int index);

    // rect
    float GetRectX(int index);
    float GetRectY(int index);
    float GetRectWidth(int index);
    float GetRectHeight(int index);
    float GetRectRx(int index);   // bo góc rx
    float GetRectRy(int index);   // bo góc ry

    // circle
    float GetCx(int index);
    float GetCy(int index);
    float GetR(int index);

    // ellipse
    float GetRx(int index);
    float GetRy(int index);

    // line
    float GetX1(int index);
    float GetY1(int index);
    float GetX2(int index);
    float GetY2(int index);

    // polygon / polyline
    std::vector<std::pair<float, float>> GetPoints(int index); // giữ API cũ

    // path
    std::string GetPathD(int index); // chuỗi 'd' nguyên bản (bạn render bằng path parser riêng)
	float GetMiterLimit(int index); // giữ API cũ

    // image (nếu muốn hỗ trợ <image>)
    std::string GetImageHref(int index); // xlink:href hoặc href
    float       GetImageWidth(int index);
    float       GetImageHeight(int index);

    // ========= Text =========
    // Nội dung text giữ ở ATTRIBUTE::_text (đã có). Bổ sung font & layout:
    float GetDx(int index);
    float GetDy(int index);
    std::string GetFontFamily(int index);       // chuỗi, vd "Times New Roman"
    float       GetFontSize(int index);         // px (parse từ "12", "12px", "1.2em" → khuyến nghị quy đổi px trước)
    std::string GetFontWeight(int index);       // "normal" | "bold" | "100".."900"
    std::string GetFontStyle(int index);        // "normal" | "italic" | "oblique"
    std::string GetTextAnchor(int index);       // "start" | "middle" | "end"
    std::string GetDominantBaseline(int index); // "alphabetic" | "hanging" ... (tùy chọn)
    float       GetLetterSpacing(int index);    // px (nếu có)
    float       GetWordSpacing(int index);      // px (nếu có)

    // ========= Tiện ích khác =========
    // Trả kích thước nếu có thể suy ra (vd với rect, image)
    std::pair<float, float> GetSize(int index); // giữ API cũ
    std::string GetAttrRaw(int i, const char* key);
    std::string FindInStyle(int i, const char* prop);
    bool IsFillNone(int i);

    // ========= Gradient =========
    // Thêm biến lưu danh sách các gradient đã đọc được

    // Hàm lấy gradient (đã có trong code bạn, nhưng check lại signature)
    bool TryGetLinearGradient(const std::string& id, LinearGradientDef& out) const;
    bool TryGetRadialGradient(const std::string& id, RadialGradientDef& out) const;
    // ========= Helper cho transform và kế thừa từ <g> =========
    // Lấy danh sách transform operations tích lũy từ root đến node i
    std::vector<TransformOperation> GetAccumulatedTransformOperations(int index) const;
    // Lấy thuộc tính kế thừa từ các <g> cha (stroke, fill, font-size, ...)
    std::string GetInheritedAttribute(int index, const std::string& key) const;
    
    // ========= Public wrappers cho ParseFloat và ParseColor =========
    float ParseFloatPublic(const std::string& s, float def = 0.0f) const;
    std::vector<int> ParseColorPublic(const std::string& s) const;
};

#endif // !READSVG_H
