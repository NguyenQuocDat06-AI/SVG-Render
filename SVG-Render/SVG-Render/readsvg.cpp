// =========================
// BỔ SUNG HÀM CÒN THIẾU
// (chèn dưới các hàm bạn đã có)
// =========================



#include <tuple>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cstring>

#include "readsvg.h"
namespace {
    inline bool is_space(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
    }
    std::string trim(const std::string& s) {
        size_t l = 0, r = s.size();
        while (l < r && is_space(s[l])) ++l;
        while (r > l && is_space(s[r - 1])) --r;
        return s.substr(l, r - l);
    }
    std::string lower(std::string s) {
        for (auto& ch : s) ch = (char)std::tolower((unsigned char)ch);
        return s;
    }
    std::string trimLower(std::string s) {
        return lower(trim(s));
    }

    // lấy attribute (nếu có) từ map
    std::string getAttr(const ATTRIBUTE& n, const char* key) {
        auto it = n._att.find(key);
        return (it == n._att.end()) ? std::string() : it->second;
    }

    // tìm trong style="...": prop phải có dấu ':' sau tên (vd "fill:")
    std::string findInStyle(const ATTRIBUTE& n, const char* prop) {
        auto it = n._att.find("style");
        if (it == n._att.end()) return {};
        std::string s = it->second;
        auto p = s.find(prop);
        if (p == std::string::npos) return {};
        p += std::strlen(prop);
        auto q = s.find(';', p);
        std::string v = s.substr(p, (q == std::string::npos) ? std::string::npos : q - p);
        return trim(v);
    }

    inline int clamp255i(int v) {
        return (v < 0) ? 0 : (v > 255 ? 255 : v);
    }
}

// Khai báo trước để dùng trong TraverseNode
static std::map<std::string, std::string> _parseStyleKV(const std::string& style);

READSVG::READSVG() {};
READSVG::~READSVG() {};
vector<char> READSVG::ReadFileToBuffer(const string& path) {
    ifstream fin(path, ios::binary);
    if (!fin) throw runtime_error("Khong mo duoc file: " + path);

    fin.seekg(0, ios::end);
    streamsize size = fin.tellg();
    if (size < 0) throw runtime_error("Khong doc duoc kich thuoc file: " + path);
    fin.seekg(0, ios::beg);

    vector<char> buffer(static_cast<size_t>(size));
    if (!fin.read(buffer.data(), size)) {
        throw runtime_error("Loi khi doc file: " + path);
    }
    buffer.push_back('\0');
    return buffer;
}

static void _matMul(float M[6], const float T[6]) {
    float a = M[0] * T[0] + M[2] * T[1];
    float b = M[1] * T[0] + M[3] * T[1];
    float c = M[0] * T[2] + M[2] * T[3];
    float d = M[1] * T[2] + M[3] * T[3];
    float e = M[0] * T[4] + M[2] * T[5] + M[4];
    float f = M[1] * T[4] + M[3] * T[5] + M[5];
    M[0] = a; M[1] = b; M[2] = c; M[3] = d; M[4] = e; M[5] = f;
}

void READSVG::ResolveGradientStops(const std::string& hrefId, std::vector<float>& outOffsets, std::vector<std::vector<int>>& outColors, int depth) const {
    if (depth > 5) return; // Tránh vòng lặp vô tận

    // 1. Thử tìm trong Linear Map
    auto itL = _linearGradients.find(hrefId);
    if (itL != _linearGradients.end()) {
        if (!itL->second.colors.empty()) {
            outOffsets = itL->second.offsets;
            outColors = itL->second.colors;
            return;
        }
        // Nếu Linear này cũng rỗng nhưng có href -> đệ quy tiếp
        if (!itL->second.href.empty()) {
            ResolveGradientStops(itL->second.href, outOffsets, outColors, depth + 1);
            return;
        }
    }

    // 2. Thử tìm trong Radial Map
    auto itR = _radialGradients.find(hrefId);
    if (itR != _radialGradients.end()) {
        if (!itR->second.colors.empty()) {
            outOffsets = itR->second.offsets;
            outColors = itR->second.colors;
            return;
        }
        if (!itR->second.href.empty()) {
            ResolveGradientStops(itR->second.href, outOffsets, outColors, depth + 1);
            return;
        }
    }
}


bool READSVG::TryGetRadialGradient(const std::string& id, RadialGradientDef& out) const {
    auto it = _radialGradients.find(id);
    if (it == _radialGradients.end()) return false;
    out = it->second;

    // Nếu không có stops nhưng có href -> Đi tìm stops từ cha
    if (out.colors.empty() && !out.href.empty()) {
        const_cast<READSVG*>(this)->ResolveGradientStops(out.href, out.offsets, out.colors, 0);
    }
    return true;
}

// Duyệt đệ quy một node và toàn bộ con của nó, lưu vào _node
void READSVG::TraverseNode(xml_node<>* node, int parentIndex, int depth) {
    if (!node) return;

    if (node->type() == node_element) {
        // 1. Lấy Attributes
        map<string, string> _temp;
        for (xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
            _temp[attr->name()] = attr->value();
        }

        // 2. Lấy Text Content (Gom tất cả node con)
        std::string textValue;
        if (std::strcmp(node->name(), "text") == 0) {
            for (xml_node<>* child = node->first_node(); child; child = child->next_sibling()) {
                if (child->type() == node_data && child->value()) {
                    textValue += child->value();
                }
            }
        }
        else {
            textValue = node->value() ? node->value() : "";
        }

        // 3. Tạo Node
        int currentIndex = (int)_node.size();
        ATTRIBUTE attrNode{ node->name(), textValue, _temp, parentIndex, depth };
        _node.push_back(attrNode);

        // 4. Đệ quy duyệt con
        for (xml_node<>* child = node->first_node(); child; child = child->next_sibling()) {
            TraverseNode(child, currentIndex, depth + 1);
        }

        // 5. Parse Gradient (Linear & Radial)

        // Helper: Lấy giá trị float, xử lý %
        auto getValPercent = [&](const char* key, float defVal) {
            std::string v = getAttr(attrNode, key);
            if (v.empty()) return defVal;
            float val = ParseFloat(v, defVal);
            if (v.back() == '%') val /= 100.0f;
            return val;
            };

        // --- A. LINEAR GRADIENT ---
        if (attrNode._tags == "linearGradient") {
            std::string id = getAttr(attrNode, "id");
            if (!id.empty()) {
                LinearGradientDef def;
                def.id = id;

                def.href = getAttr(attrNode, "href");
                if (def.href.empty()) def.href = getAttr(attrNode, "xlink:href");
                // Xóa dấu # ở đầu nếu có
                if (!def.href.empty() && def.href[0] == '#') def.href.erase(0, 1);

                def.x1 = getValPercent("x1", 0.0f);
                def.y1 = getValPercent("y1", 0.0f);
                def.x2 = getValPercent("x2", 1.0f);
                def.y2 = getValPercent("y2", 0.0f);
                def.units = getAttr(attrNode, "gradientUnits");
                def.spreadMethod = getAttr(attrNode, "spreadMethod");

                // === Xử lý gradientTransform ===
                std::string gradTransStr = getAttr(attrNode, "gradientTransform");
                if (!gradTransStr.empty()) {
                    auto ops = ParseTransformOperations(gradTransStr);
                    float m[6] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
                    for (const auto& op : ops) {
                        float t[6] = { 1, 0, 0, 1, 0, 0 };
                        if (op.type == TransformType::Matrix) {
                            std::memcpy(t, op.values, 6 * sizeof(float));
                        }
                        else if (op.type == TransformType::Translate) {
                            t[4] = op.values[0]; t[5] = op.values[1];
                        }
                        else if (op.type == TransformType::Scale) {
                            t[0] = op.values[0]; t[3] = op.values[1];
                        }
                        else if (op.type == TransformType::Rotate) {
                            float rad = op.values[0] * 3.1415926535f / 180.0f;
                            float c = std::cos(rad), s = std::sin(rad);
                            t[0] = c; t[1] = s; t[2] = -s; t[3] = c;
                        }
                        _matMul(m, t);
                    }
                    def.gradientTransform.assign(m, m + 6);
                }

                // Parse Stops
                for (xml_node<>* s = node->first_node("stop"); s; s = s->next_sibling("stop")) {
                    std::map<std::string, std::string> stopAtt;
                    for (xml_attribute<>* a = s->first_attribute(); a; a = a->next_attribute())
                        stopAtt[a->name()] = a->value();

                    std::map<std::string, std::string> styleMap;
                    auto itStyle = stopAtt.find("style");
                    if (itStyle != stopAtt.end()) styleMap = _parseStyleKV(itStyle->second);

                    auto getValue = [&](const std::string& key) -> std::string {
                        if (styleMap.count(key)) return styleMap[key];
                        if (stopAtt.count(key)) return stopAtt[key];
                        return "";
                        };

                    std::string offStr = getValue("offset");
                    float off = 0.0f;
                    if (!offStr.empty()) {
                        off = ParseFloat(offStr, 0.0f);
                        if (offStr.back() == '%') off /= 100.0f;
                    }

                 
                    float opacity = 1.0f;
                    std::string opStr = getValue("stop-opacity");
                    if (!opStr.empty()) opacity = ParseOpacity(opStr, 1.0f);

                    std::string colorStr = getValue("stop-color");
                    auto rgb = ParseColor(colorStr);
                    if (rgb.empty() && !colorStr.empty() && colorStr != "none") rgb = { 0, 0, 0 };

                    if (!rgb.empty()) {
                        rgb.push_back(static_cast<int>(opacity * 255.0f));

                        def.offsets.push_back(off);
                        def.colors.push_back(rgb);
                    }
                }
                _linearGradients[id] = def;
            }
        }
        // --- B. RADIAL GRADIENT ---
        else if (attrNode._tags == "radialGradient") {
            std::string id = getAttr(attrNode, "id");
            if (!id.empty()) {
                RadialGradientDef def;
                def.id = id;

                def.href = getAttr(attrNode, "href");
                if (def.href.empty()) def.href = getAttr(attrNode, "xlink:href");
                if (!def.href.empty() && def.href[0] == '#') def.href.erase(0, 1);

                // FIX CRITICAL: Hàm parse giá trị phần trăm
                auto getValPercent = [&](const char* key, float defVal) -> float {
                    std::string v = getAttr(attrNode, key);
                    if (v.empty()) return defVal;

                    // Kiểm tra có dấu % không
                    bool hasPercent = (!v.empty() && v.back() == '%');

                    // Parse số (bỏ dấu % nếu có)
                    std::string numStr = hasPercent ? v.substr(0, v.size() - 1) : v;
                    float val = ParseFloat(numStr, defVal);

                    // QUAN TRỌNG: Nếu có %, chia 100 để ra tỷ lệ
                    // Ví dụ: "210%" -> 210 / 100 = 2.1
                    //        "-100%" -> -100 / 100 = -1.0
                    if (hasPercent) {
                        val /= 100.0f;
                    }

                    return val;
                    };

                // Parse các thuộc tính (mặc định theo SVG spec)
                def.cx = getValPercent("cx", 0.5f);  // 50%
                def.cy = getValPercent("cy", 0.5f);  // 50%
                def.r = getValPercent("r", 0.5f);  // 50%
                def.fx = getValPercent("fx", def.cx); // Mặc định = cx
                def.fy = getValPercent("fy", def.cy); // Mặc định = cy

                def.units = getAttr(attrNode, "gradientUnits");
                def.spreadMethod = getAttr(attrNode, "spreadMethod");

                // === Xử lý gradientTransform ===
                std::string gradTransStr = getAttr(attrNode, "gradientTransform");
                if (!gradTransStr.empty()) {
                    auto ops = ParseTransformOperations(gradTransStr);
                    float m[6] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
                    for (const auto& op : ops) {
                        float t[6] = { 1, 0, 0, 1, 0, 0 };
                        if (op.type == TransformType::Matrix) {
                            std::memcpy(t, op.values, 6 * sizeof(float));
                        }
                        else if (op.type == TransformType::Translate) {
                            t[4] = op.values[0]; t[5] = op.values[1];
                        }
                        else if (op.type == TransformType::Scale) {
                            t[0] = op.values[0]; t[3] = op.values[1];
                        }
                        else if (op.type == TransformType::Rotate) {
                            float rad = op.values[0] * 3.1415926535f / 180.0f;
                            float c = std::cos(rad), s = std::sin(rad);
                            t[0] = c; t[1] = s; t[2] = -s; t[3] = c;
                        }
                        _matMul(m, t);
                    }
                    def.gradientTransform.assign(m, m + 6);
                }

                // Parse Stops
                for (xml_node<>* s = node->first_node("stop"); s; s = s->next_sibling("stop")) {
                    std::map<std::string, std::string> stopAtt;
                    for (xml_attribute<>* a = s->first_attribute(); a; a = a->next_attribute())
                        stopAtt[a->name()] = a->value();

                    std::map<std::string, std::string> styleMap;
                    auto itStyle = stopAtt.find("style");
                    if (itStyle != stopAtt.end()) styleMap = _parseStyleKV(itStyle->second);

                    auto getValue = [&](const std::string& key) -> std::string {
                        if (styleMap.count(key)) return styleMap[key];
                        if (stopAtt.count(key)) return stopAtt[key];
                        return "";
                        };

                    // Parse offset
                    std::string offStr = getValue("offset");
                    float off = 0.0f;
                    if (!offStr.empty()) {
                        off = ParseFloat(offStr, 0.0f);
                        if (offStr.back() == '%') off /= 100.0f;
                    }

                    // Parse opacity
                    float opacity = 1.0f;
                    std::string opStr = getValue("stop-opacity");
                    if (!opStr.empty()) opacity = ParseOpacity(opStr, 1.0f);

                    // Parse color
                    std::string colorStr = getValue("stop-color");
                    auto rgb = ParseColor(colorStr);
                    if (rgb.empty() && !colorStr.empty() && colorStr != "none")
                        rgb = { 0, 0, 0 };

                    if (!rgb.empty()) {
                        rgb.push_back(static_cast<int>(opacity * 255.0f));
                        def.offsets.push_back(off);
                        def.colors.push_back(rgb);
                    }
                }
                _radialGradients[id] = def;
            }
        }
    }
}
void READSVG::ParseFromBuffer(const string& _path) {
    vector <char> xml(ReadFileToBuffer(_path));
    xml_document<> doc;
    doc.parse<0>(xml.data()); // Chú ý: RapidXML sẽ sửa đổi buffer (thêm '\0' tạm)
    // 3) Lấy root <svg>
    xml_node<>* root = doc.first_node("svg");
    if (!root) {
        throw runtime_error("Khong tim thay the <svg> o root");
    }

    // 4) Duyệt đệ quy toàn bộ cây (svg, g, defs, path, rect, ...)
    _node.clear();
    TraverseNode(root, -1, 0);
}
static std::map<std::string, std::string> LoadColorMap(const std::string& path) {
    std::ifstream fin(path);
    if (!fin) throw std::runtime_error("Cannot open file: " + path);

    std::map<std::string, std::string> mp;
    std::string line;

    while (std::getline(fin, line)) {
        size_t tabPos = line.find('\t');
        if (tabPos != std::string::npos) {
            std::string name = line.substr(0, tabPos);
            std::string hex = line.substr(tabPos + 1);
            mp[name] = hex;
        }
    }
    return mp;
}
map<string, string> gNameColor(LoadColorMap("color.txt"));
std::vector<int> READSVG::getColor(const std::string& raw) {
    std::vector<int> rgb; // rỗng = "không có màu"
    if (raw.empty()) return rgb;

    // 1) Chuẩn hoá đầu vào
    std::string key = trimLower(raw);

    // 2) Nếu có tên màu trong bảng -> thay bằng giá trị đã map (vd "#acc2d9")
    //    Không dùng operator[] để tránh tạo key rỗng ngoài ý muốn.
    auto it = gNameColor.find(key);
    std::string s = (it != gNameColor.end() && !it->second.empty())
        ? trimLower(it->second)   // dùng giá trị trong file
        : key;                    // dùng chính chuỗi đã chuẩn hoá

    // 3) Các trường hợp đặc biệt
    if (s.empty() || s == "none") return rgb;

    // 4) #RRGGBB hoặc #RGB
    if (!s.empty() && s[0] == '#') {
        if (s.size() == 7) { // #RRGGBB
            int r = std::stoi(s.substr(1, 2), nullptr, 16);
            int g = std::stoi(s.substr(3, 2), nullptr, 16);
            int b = std::stoi(s.substr(5, 2), nullptr, 16);
            rgb = { clamp255i(r), clamp255i(g), clamp255i(b) };
        }   
        else if (s.size() == 4) { // #RGB
            auto dup = [](char c) { return std::string(2, c); };
            int r = std::stoi(dup(s[1]), nullptr, 16);
            int g = std::stoi(dup(s[2]), nullptr, 16);
            int b = std::stoi(dup(s[3]), nullptr, 16);
            rgb = { clamp255i(r), clamp255i(g), clamp255i(b) };
        }
        return rgb;
    }

    // 5) rgb(r,g,b) hoặc rgb(r, g, b) – parse linh hoạt khoảng trắng
    if (s.rfind("rgb(", 0) == 0 && s.back() == ')') {
        std::string t = s.substr(4, s.size() - 5); // bên trong ngoặc
        // thay dấu phẩy thành khoảng trắng rồi dùng stringstream
        for (char& c : t) {
            if (c == ',') c = ' ';
        }
        int r = 0, g = 0, b = 0;
        std::stringstream ss(t);
        if (ss >> r >> g >> b) {
            rgb = { clamp255i(r), clamp255i(g), clamp255i(b) };
            return rgb;
        }
    }

    // 6) Không parse được -> xem như không có màu
    return rgb;
}
ATTRIBUTE READSVG::GetNode(int index) {
    if (index < 0 || index >= _node.size()) {
        throw out_of_range("Index out of range in GetNode");
    }
    return _node[index];
}
std::vector<int> READSVG::GetFill(int index) {
    // Thay vì chỉ tìm ở node hiện tại, hãy tìm ngược lên cha
    std::string v = GetInheritedAttribute(index, "fill");
    return getColor(v);
}
float READSVG::GetHeight(int index) {
    string str = _node[index]._att["height"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetWidth(int index) {
    string str = _node[index]._att["width"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
std::vector<int> READSVG::GetStroke(int index) {
    std::string v = GetInheritedAttribute(index, "stroke");
    return getColor(v);
}
float READSVG::GetStrokeWidth(int index) {
    // Stroke-width cũng kế thừa
    std::string v = GetInheritedAttribute(index, "stroke-width");
    return ParseFloat(v, 1.0f); // Mặc định 1.0 nếu tìm mãi không thấy
}
float READSVG::GetStrokeOpacity(int index) {
    // Lấy thuộc tính stroke-opacity (có kế thừa từ cha)
    std::string v = GetInheritedAttribute(index, "stroke-opacity");
    // ParseOpacity đã có sẵn trong code của bạn (trả về 1.0f nếu rỗng)
    return ParseOpacity(v, 1.0f);
}
float READSVG::GetFillOpacity(int index) {
    std::string v = GetInheritedAttribute(index, "fill-opacity");
    return ParseOpacity(v, 1.0f);
}
float READSVG::GetX(int index) {
    string str = _node[index]._att["x"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetY(int index) {
    string str = _node[index]._att["y"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetCx(int index) {
    string str = _node[index]._att["cx"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetCy(int index) {
    string str = _node[index]._att["cy"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetRx(int index) {
    string str = _node[index]._att["rx"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetRy(int index) {
    string str = _node[index]._att["ry"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetR(int index) {
    string str = _node[index]._att["r"];
    if (str.empty()) return 0.0f; // Mặc định là 0.0 nếu không có thuộc tính
    return stof(str);
}
vector<pair<float, float>> READSVG::GetPoints(int index) {
    string str = _node[index]._att["points"];
    vector<pair<float, float>> points;
    size_t pos = 0;
    while (pos < str.length()) {
        // Tìm dấu phẩy hoặc khoảng trắng
        size_t commaPos = str.find(',', pos);
        size_t spacePos = str.find(' ', pos);
        size_t sepPos = min(commaPos, spacePos);
        if (sepPos == string::npos) break; // Không còn dấu phân cách
        // Lấy x
        string xStr = str.substr(pos, sepPos - pos);
        float x = stof(xStr);
        // Cập nhật vị trí
        pos = sepPos + 1;
        // Tìm dấu phẩy hoặc khoảng trắng tiếp theo cho y
        commaPos = str.find(',', pos);
        spacePos = str.find(' ', pos);
        sepPos = min(commaPos, spacePos);
        if (sepPos == string::npos) {
            // Lấy y cuối cùng
            string yStr = str.substr(pos);
            float y = stof(yStr);
            points.push_back({ x, y });
            break;
        }
        // Lấy y
        string yStr = str.substr(pos, sepPos - pos);
        float y = stof(yStr);
        points.push_back({ x, y });
        // Cập nhật vị trí
        pos = sepPos + 1;
    }
    return points;
}

// ---------- helpers cục bộ ----------
static std::string _trim(std::string s) {
    auto issp = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && issp((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && issp((unsigned char)s.back()))  s.pop_back();
    return s;
}
static bool _ieq(const std::string& a, const std::string& b) {
#ifdef _WIN32
    return _stricmp(a.c_str(), b.c_str()) == 0;
#else
    if (a.size() != b.size()) return false;
    for (size_t i = 0;i < a.size();++i) if (std::tolower(a[i]) != std::tolower(b[i])) return false;
    return true;
#endif
}
// tách style="a:b; c:d"
static std::map<std::string, std::string> _parseStyleKV(const std::string& style) {
    std::map<std::string, std::string> kv;
    size_t i = 0, n = style.size();
    while (i < n) {
        size_t kEnd = style.find(':', i);
        if (kEnd == std::string::npos) break;
        std::string key = _trim(style.substr(i, kEnd - i));
        size_t vStart = kEnd + 1;
        size_t vEnd = style.find(';', vStart);
        std::string val = _trim(vEnd == std::string::npos ? style.substr(vStart)
            : style.substr(vStart, vEnd - vStart));
        if (!key.empty()) kv[key] = val;
        if (vEnd == std::string::npos) break;
        i = vEnd + 1;
        while (i < n && std::isspace((unsigned char)style[i])) ++i;
    }
    return kv;
}
static std::string _getAttrWithStyle(const ATTRIBUTE& node, const std::string& key) {
    auto itS = node._att.find("style");
    if (itS != node._att.end()) {
        auto kv = _parseStyleKV(itS->second);
        auto it = kv.find(key);
        if (it != kv.end()) return it->second;
    }
    auto it = node._att.find(key);
    return (it != node._att.end()) ? it->second : std::string();
}
// tách list float theo ,/space
static std::vector<float> _splitFloatList(const std::string& s) {
    std::vector<float> out;
    size_t i = 0, n = s.size();
    auto isSep = [&](char c) { return c == ',' || std::isspace((unsigned char)c); };
    while (i < n) {
        while (i < n && isSep(s[i])) ++i;
        if (i >= n) break;
        size_t j = i;
        while (j < n && !isSep(s[j])) ++j;
        try { out.push_back(std::stof(s.substr(i, j - i))); }
        catch (...) {}
        i = j;
    }
    return out;
}

// ---------- định nghĩa các static function trong class ----------

float READSVG::ParseFloat(const std::string& s, float def) {
    if (s.empty()) return def;
    try {
        size_t i = 0;
        // đọc phần số, bỏ hậu tố đơn vị (px, %, ...)
        while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E')) ++i;
        return std::stof(s.substr(0, i));
    }
    catch (...) { return def; }
}

std::vector<int> READSVG::ParseColor(const std::string& s) {
    // dùng lại getColor() của bạn cho đơn giản
    READSVG tmp;
    return tmp.getColor(s);
}

float READSVG::ParseOpacity(const std::string& s, float def) {
    if (s.empty()) return def;
    try {
        float v = std::stof(s);
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
        return v;
    }
    catch (...) { return def; }
}

std::vector<float> READSVG::ParseFloatList(const std::string& s) {
    return _splitFloatList(s);
}

std::pair<float, float> READSVG::ParsePair(const std::string& sx, const std::string& sy, float defx, float defy) {
    return { ParseFloat(sx,defx), ParseFloat(sy,defy) };
}

// nhân ma trận 2D SVG [a b c d e f]: M = M * T


std::vector<TransformOperation> READSVG::ParseTransformOperations(const std::string& s) {
    std::vector<TransformOperation> operations;
    std::string t = _trim(s);
    if (t.empty()) return operations;

    size_t i = 0, n = t.size();
    auto isAlpha = [&](char c) { return std::isalpha((unsigned char)c) != 0; };

    while (i < n) {
        // 1. Bỏ qua khoảng trắng đầu
        while (i < n && std::isspace((unsigned char)t[i])) ++i;
        if (i >= n) break;

        // =========================================================
        // [FIX QUAN TRỌNG] Bỏ qua dấu phẩy (nếu có) giữa các lệnh
        // =========================================================
        if (t[i] == ',') {
            ++i; // Nhảy qua dấu phẩy
            // Bỏ qua tiếp khoảng trắng sau dấu phẩy (nếu có)
            while (i < n && std::isspace((unsigned char)t[i])) ++i;
            if (i >= n) break;
        }

        // 2. Đọc tên hàm (translate, rotate...)
        size_t j = i;
        while (j < n && isAlpha(t[j])) ++j;

        // Nếu j == i nghĩa là không đọc được chữ nào -> Gặp ký tự lạ -> Skip để tránh lặp vô tận
        if (j == i) {
            ++i; continue;
        }

        std::string fn = t.substr(i, j - i);

        // Bỏ qua khoảng trắng trước dấu '('
        while (j < n && std::isspace((unsigned char)t[j])) ++j;

        // Kiểm tra dấu mở ngoặc
        if (j >= n || t[j] != '(') break;
        ++j; // Nhảy qua '('

        // 3. Tìm dấu đóng ngoặc ')' tương ứng
        size_t k = j; int paren = 1;
        while (k < n && paren > 0) {
            if (t[k] == '(') ++paren;
            else if (t[k] == ')') --paren;
            ++k;
        }
        if (paren != 0) break; // Lỗi ngoặc không cân bằng

        std::string args = t.substr(j, (k - 1) - j);
        i = k; // Cập nhật vị trí i để chuẩn bị cho vòng lặp sau

        // 4. Parse tham số bên trong và tạo Operation
        auto vals = _splitFloatList(args);
        TransformOperation op;
        std::memset(op.values, 0, sizeof(op.values));

        // ... (Phần logic switch/case của bạn GIỮ NGUYÊN) ...
        if (_ieq(fn, "matrix") && vals.size() >= 6) {
            op.type = TransformType::Matrix;
            op.values[0] = vals[0]; op.values[1] = vals[1];
            op.values[2] = vals[2]; op.values[3] = vals[3];
            op.values[4] = vals[4]; op.values[5] = vals[5];
            operations.push_back(op);
        }
        else if (_ieq(fn, "translate")) {
            op.type = TransformType::Translate;
            op.values[0] = vals.size() > 0 ? vals[0] : 0.f;
            op.values[1] = vals.size() > 1 ? vals[1] : 0.f;
            operations.push_back(op);
        }
        else if (_ieq(fn, "scale")) {
            op.type = TransformType::Scale;
            // Scale(x) tương đương Scale(x, x)
            op.values[0] = vals.size() > 0 ? vals[0] : 1.f;
            op.values[1] = vals.size() > 1 ? vals[1] : op.values[0];
            operations.push_back(op);
        }
        else if (_ieq(fn, "rotate")) {
            op.values[0] = vals.size() > 0 ? vals[0] : 0.f;
            if (vals.size() >= 3) {
                op.type = TransformType::RotateAt;
                op.values[1] = vals[1]; op.values[2] = vals[2];
            }
            else {
                op.type = TransformType::Rotate;
            }
            operations.push_back(op);
        }
        else if (_ieq(fn, "skewX")) {
            op.type = TransformType::SkewX;
            op.values[0] = vals.size() > 0 ? vals[0] : 0.f;
            operations.push_back(op);
        }
        else if (_ieq(fn, "skewY")) {
            op.type = TransformType::SkewY;
            op.values[0] = vals.size() > 0 ? vals[0] : 0.f;
            operations.push_back(op);
        }
    }
    return operations;
}
// ---------- các getter còn thiếu theo header ----------

std::string READSVG::GetTagName(int index) {
    if (index < 0 || index >= (int)_node.size()) return "";
    return _node[index]._tags;
}

std::map<std::string, std::string> READSVG::GetAttributes(int index) {
    if (index < 0 || index >= (int)_node.size()) return {};
    return _node[index]._att;
}

std::string READSVG::GetAttributeRaw(int index, const std::string& key, const std::string& def) {
    if (index < 0 || index >= (int)_node.size()) return def;
    auto it = _node[index]._att.find(key);
    return (it != _node[index]._att.end()) ? it->second : def;
}

// fill/stroke nâng cao
std::string READSVG::GetFillRule(int index) {
    std::string v = GetInheritedAttribute(index, "fill-rule");
    return v.empty() ? "nonzero" : v;
}
std::string READSVG::GetStrokeLinecap(int index) {
    auto v = _getAttrWithStyle(_node[index], "stroke-linecap");
    return v.empty() ? "butt" : v;
}
std::string READSVG::GetStrokeLinejoin(int index) {
    std::string v = GetInheritedAttribute(index, "stroke-linejoin");
    return v.empty() ? "miter" : v;
}
float READSVG::GetStrokeMiterlimit(int index) {
    std::string v = GetInheritedAttribute(index, "stroke-miterlimit");
    return ParseFloat(v, 4.0f);
}
std::vector<float> READSVG::GetStrokeDasharray(int index) {
    auto v = _getAttrWithStyle(_node[index], "stroke-dasharray");
    if (v.empty() || _ieq(v, "none")) return {};
    return ParseFloatList(v);
}
float READSVG::GetStrokeDashoffset(int index) {
    auto v = _getAttrWithStyle(_node[index], "stroke-dashoffset");
    return ParseFloat(v, 0.0f);
}

// visibility / display / opacity
bool READSVG::GetDisplayNone(int index) {
    auto v = _getAttrWithStyle(_node[index], "display");
    return _ieq(v, "none");
}
bool READSVG::GetVisible(int index) {
    auto v = _getAttrWithStyle(_node[index], "visibility");
    if (_ieq(v, "hidden") || _ieq(v, "collapse")) return false;
    return true;
}
float READSVG::GetOpacity(int index) {
    auto v = _getAttrWithStyle(_node[index], "opacity");
    return ParseOpacity(v, 1.0f);
}

// transform
bool READSVG::HasTransform(int index) {
    auto v = _getAttrWithStyle(_node[index], "transform");
    return !_trim(v).empty();
}


std::vector<TransformOperation> READSVG::GetTransformOperations(int index) {
    auto v = _getAttrWithStyle(_node[index], "transform");
    return ParseTransformOperations(v);
}

// id/class/style
std::string READSVG::GetId(int index) {
    if (index < 0 || index >= (int)_node.size()) return "";
    auto it = _node[index]._att.find("id");
    return it != _node[index]._att.end() ? it->second : "";
}
std::string READSVG::GetClass(int index) {
    if (index < 0 || index >= (int)_node.size()) return "";
    auto it = _node[index]._att.find("class");
    return it != _node[index]._att.end() ? it->second : "";
}
std::string READSVG::GetStyle(int index) {
    if (index < 0 || index >= (int)_node.size()) return "";
    auto it = _node[index]._att.find("style");
    return it != _node[index]._att.end() ? it->second : "";
}

// canvas-level
bool READSVG::HasViewBox(int index) const {
    auto it = _node[index]._att.find("viewBox");
    return it != _node[index]._att.end() && !_trim(it->second).empty();
}
std::tuple<float, float, float, float> READSVG::GetViewBox(int index) const  {
    float x = 0, y = 0, w = 0, h = 0;
    auto it = _node[index]._att.find("viewBox");
    if (it != _node[index]._att.end()) {
        auto v = _splitFloatList(it->second);
        if (v.size() >= 4) { x = v[0]; y = v[1]; w = v[2]; h = v[3]; }
    }
    return std::make_tuple(x, y, w, h);
}
std::string READSVG::GetPreserveAspectRatio(int index) {
    auto it = _node[index]._att.find("preserveAspectRatio");
    return it != _node[index]._att.end() ? it->second : "xMidYMid meet";
}

// rect (tên đã có trong header)
float READSVG::GetRectX(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "x"), 0.f); }
float READSVG::GetRectY(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "y"), 0.f); }
float READSVG::GetRectWidth(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "width"), 0.f); }
float READSVG::GetRectHeight(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "height"), 0.f); }
float READSVG::GetRectRx(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "rx"), 0.f); }
float READSVG::GetRectRy(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "ry"), 0.f); }

// line
float READSVG::GetX1(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "x1"), 0.f); }
float READSVG::GetY1(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "y1"), 0.f); }
float READSVG::GetX2(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "x2"), 0.f); }
float READSVG::GetY2(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "y2"), 0.f); }

// path
std::string READSVG::GetPathD(int index) {
    return _getAttrWithStyle(_node[index], "d");
}

// image
std::string READSVG::GetImageHref(int index) {
    // ưu tiên href, sau đó xlink:href
    auto itH = _node[index]._att.find("href");
    if (itH != _node[index]._att.end()) return itH->second;
    auto itX = _node[index]._att.find("xlink:href");
    return (itX != _node[index]._att.end()) ? itX->second : "";
}
float READSVG::GetImageWidth(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "width"), 0.f); }
float READSVG::GetImageHeight(int index) { return ParseFloat(_getAttrWithStyle(_node[index], "height"), 0.f); }

// text/font
std::string READSVG::GetFontFamily(int index) {
    std::string v = GetInheritedAttribute(index, "font-family");
    return v.empty() ? "Times New Roman" : v; // Default font
}
float READSVG::GetFontSize(int index) {
    std::string v = GetInheritedAttribute(index, "font-size");
    return ParseFloat(v, 16.0f); // Default size
}
std::string READSVG::GetFontWeight(int index) {
    std::string v = GetInheritedAttribute(index, "font-weight");
    return v.empty() ? "normal" : v;
}
std::string READSVG::GetFontStyle(int index) {
    auto v = _getAttrWithStyle(_node[index], "font-style");
    return v.empty() ? "normal" : v;
}
std::string READSVG::GetTextAnchor(int index) {
    std::string v = GetInheritedAttribute(index, "text-anchor");
    return v.empty() ? "start" : v;
}
std::string READSVG::GetDominantBaseline(int index) {
    auto v = _getAttrWithStyle(_node[index], "dominant-baseline");
    return v.empty() ? "alphabetic" : v;
}
float READSVG::GetLetterSpacing(int index) {
    auto v = _getAttrWithStyle(_node[index], "letter-spacing");
    return ParseFloat(v, 0.f);
}
float READSVG::GetWordSpacing(int index) {
    auto v = _getAttrWithStyle(_node[index], "word-spacing");
    return ParseFloat(v, 0.f);
}

float READSVG::GetDx(int index) {
    // dx có thể nằm trong style hoặc attribute
    return ParseFloat(_getAttrWithStyle(_node[index], "dx"), 0.0f);
}

float READSVG::GetDy(int index) {
    return ParseFloat(_getAttrWithStyle(_node[index], "dy"), 0.0f);
}
// Tiện ích
std::pair<float, float> READSVG::GetSize(int index) {
    const auto& tag = _node[index]._tags;
    if (tag == "rect" || tag == "image" || tag == "svg") {
        return { GetWidth(index), GetHeight(index) };
    }
    else if (tag == "circle") {
        float d = 2.f * GetR(index);
        return { d, d };
    }
    else if (tag == "ellipse") {
        return { 2.f * GetRx(index), 2.f * GetRy(index) };
    }
    return { 0.f, 0.f };
}

string READSVG::GetAttrRaw(int i, const char* key) {
    auto att = GetAttributes(i);
    auto it = att.find(key);
    return (it == att.end()) ? std::string() : it->second;
}

string READSVG::FindInStyle(int i, const char* prop) {
    auto att = GetAttributes(i);
    auto it = att.find("style");
    if (it == att.end()) return {};
    const std::string& s = it->second;
    auto p = s.find(prop);
    if (p == std::string::npos) return {};
    p += std::strlen(prop);
    auto q = s.find(';', p);
    std::string v = s.substr(p, (q == std::string::npos) ? std::string::npos : q - p);
    // trim nhẹ
    auto l = v.find_first_not_of(" \t\r\n");
    auto r = v.find_last_not_of(" \t\r\n");
    return (l == std::string::npos) ? std::string() : v.substr(l, r - l + 1);
}
bool READSVG::IsFillNone(int i) {
    std::string v = GetInheritedAttribute(i, "fill");

    if (v.empty()) return false; 

    std::transform(v.begin(), v.end(), v.begin(),
        [](unsigned char c) { return std::tolower(c); });

    return v == "none";
}

bool READSVG::TryGetLinearGradient(const std::string& id, LinearGradientDef& out) const {
    auto it = _linearGradients.find(id);
    if (it == _linearGradients.end()) return false;
    out = it->second;

    // Nếu không có stops nhưng có href -> Đi tìm stops từ cha
    if (out.colors.empty() && !out.href.empty()) {
        // const_cast để gọi hàm helper (hoặc copy logic vào đây)
        const_cast<READSVG*>(this)->ResolveGradientStops(out.href, out.offsets, out.colors, 0);
    }
    return true;
}

std::vector<TransformOperation> READSVG::GetAccumulatedTransformOperations(int index) const {
    std::vector<TransformOperation> operations;
    int current = index;
    while (current >= 0) {
        const auto& node = _node[current];
        if (node._tags == "g" || node._tags == "svg") {
            std::string transformStr = _getAttrWithStyle(node, "transform");
            auto ops = ParseTransformOperations(transformStr);
            // Thêm vào đầu danh sách (cha trước, con sau)
            operations.insert(operations.begin(), ops.begin(), ops.end());
        }
        current = node._parentIndex;
    }
    return operations;
}

std::string READSVG::GetInheritedAttribute(int index, const std::string& key) const {
    int current = index;
    while (current >= 0) {
        const auto& node = _node[current];
        auto it = node._att.find(key);
        if (it != node._att.end() && !it->second.empty()) {
            return it->second;
        }
        // Kiểm tra trong style
        auto itStyle = node._att.find("style");
        if (itStyle != node._att.end()) {
            auto kv = _parseStyleKV(itStyle->second);
            auto itKV = kv.find(key);
            if (itKV != kv.end() && !itKV->second.empty()) {
                return itKV->second;
            }
        }
        current = node._parentIndex;
    }
    return "";
}

float READSVG::ParseFloatPublic(const std::string& s, float def) const {
    return ParseFloat(s, def);
}

std::vector<int> READSVG::ParseColorPublic(const std::string& s) const {
    return ParseColor(s);
}