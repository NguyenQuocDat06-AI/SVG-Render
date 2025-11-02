#ifndef READSVG_H
#define READSVG_H
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include "rapidxml.hpp"

using namespace rapidxml;
using namespace std;

struct ATTRIBUTE {
    std::string _tags = "";             // tên thẻ: svg, g, rect, circle, text, line, polyline, polygon, path, ellipse, defs, use, ...
    std::string _text = "";             // text content (nội dung text giữa cặp thẻ) – với <text>, <tspan>
    std::map<std::string, std::string> _att; // map thuộc tính (đã giữ nguyên dạng chuỗi)
};

class READSVG {
private:
    std::vector<ATTRIBUTE> _node;

    // ========= Helpers private (khuyến nghị) =========
    std::vector<char> ReadFileToBuffer(const std::string& _path);
    static float ParseFloat(const std::string& s, float def = 0.0f);
    static std::vector<int> ParseColor(const std::string& s);                 // hỗ trợ #rgb/#rrggbb/rgb()/named-color
    static float ParseOpacity(const std::string& s, float def = 1.0f);        // clamp 0..1
    static std::vector<float> ParseFloatList(const std::string& s);           // dùng cho points, dasharray
    static std::vector<float> ParseTransformMatrix(const std::string& s);     // trả về [a,b,c,d,e,f] nếu có; rỗng nếu không parse được
    static std::pair<float, float> ParsePair(const std::string& sx, const std::string& sy, float defx = 0, float defy = 0);

public:
    READSVG();
    ~READSVG();

    // ========= Core parse =========
    void ParseFromBuffer(const std::string& _path); // Đọc file và đổ toàn bộ node vào _node
    void PrintNode();

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
    std::vector<float> GetTransformMatrix(int index);

    // id / class / style (inline CSS)
    std::string      GetId(int index);
    std::string      GetClass(int index);
    std::string      GetStyle(int index);              // nguyên chuỗi style="..."

    // ========= Canvas-level =========
    float            GetWidth(int index);              // giữ API cũ
    float            GetHeight(int index);             // giữ API cũ
    // viewBox="minX minY width height"
    bool             HasViewBox(int index);
    std::tuple<float, float, float, float> GetViewBox(int index);
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

    // image (nếu muốn hỗ trợ <image>)
    std::string GetImageHref(int index); // xlink:href hoặc href
    float       GetImageWidth(int index);
    float       GetImageHeight(int index);

    // ========= Text =========
    // Nội dung text giữ ở ATTRIBUTE::_text (đã có). Bổ sung font & layout:
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
};

#endif // !READSVG_H
