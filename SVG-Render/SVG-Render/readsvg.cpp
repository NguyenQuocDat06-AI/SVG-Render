// =========================
// BỔ SUNG HÀM CÒN THIẾU
// (chèn dưới các hàm bạn đã có)
// =========================

#include <tuple>
#include <cctype>
#include <algorithm>
#include <cmath>

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
void READSVG::ParseFromBuffer(const string& _path) {
    vector <char> xml(ReadFileToBuffer(_path));
    xml_document<> doc;
    doc.parse<0>(xml.data()); // Chú ý: RapidXML sẽ sửa đổi buffer (thêm '\0' tạm)
    // 3) Lấy root <svg>
    xml_node<>* root = doc.first_node("svg");
    if (!root) {
        throw runtime_error("Khong tim thay the <svg> o root");
    }
    // 4) Đọc một số thuộc tính thường gặp
    //const char* w = root->first_attribute("width") ? root->first_attribute("width")->value() : "(none)";
    //const char* h = root->first_attribute("height") ? root->first_attribute("height")->value() : "(none)";
    //cout << "SVG width = " << w << ", height = " << h << "\n";
    // 5) Duyet cac node con (rect, circle, line, path, ...)
    for (xml_node<>* node = root->first_node(); node; node = node->next_sibling()) {
        map<string, string> _temp;
        for (xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
            _temp[attr->name()] = attr->value();
        }
        _node.push_back(ATTRIBUTE{ node->name(), node->value(), _temp });
    }
}
void READSVG::PrintNode() {
    int _size = _node.size();
    for (int i = 0; i < _size; i++) {
        cout << "TAGS: " << _node[i]._tags << " TEXT: " << _node[i]._text << endl;
        for (auto it : _node[i]._att) {
            cout << it.first << " " << it.second << endl;
        }
    }
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

    // 5) rgb(r,g,b)
    if (s.rfind("rgb(", 0) == 0 && s.back() == ')') {
        std::string t = s.substr(4, s.size() - 5);
        int r = 0, g = 0, b = 0;
        // dùng sscanf_s hoặc stringstream đều được
        sscanf_s(t.c_str(), "%d , %d , %d", &r, &g, &b);
        rgb = { clamp255i(r), clamp255i(g), clamp255i(b) };
        return rgb;
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
    const auto& n = _node[index];
    std::string v = findInStyle(n, "fill:");
    if (v.empty()) v = getAttr(n, "fill");
    return getColor(v); // rỗng => không fill
}
float READSVG::GetFillOpacity(int index) {
    string str = _node[index]._att["fill-opacity"];
    if (str.empty()) return 1.0f; // Mặc định là 1.0 nếu không có thuộc tính
    return stof(str);
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
    const auto& n = _node[index];
    std::string v = findInStyle(n, "stroke:");
    if (v.empty()) v = getAttr(n, "stroke");
    return getColor(v); // rỗng => không stroke
}
float READSVG::GetStrokeOpacity(int index) {
    string str = _node[index]._att["stroke-opacity"];
    if (str.empty()) return 1.0f; // Mặc định là 1.0 nếu không có thuộc tính
    return stof(str);
}
float READSVG::GetStrokeWidth(int index) {
    string str = _node[index]._att["stroke-width"];
    if (str.empty()) return 1.0f; // Mặc định là 1.0 nếu không có thuộc tính
    return stof(str);
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
static void _matMul(float M[6], const float T[6]) {
    float a = M[0] * T[0] + M[2] * T[1];
    float b = M[1] * T[0] + M[3] * T[1];
    float c = M[0] * T[2] + M[2] * T[3];
    float d = M[1] * T[2] + M[3] * T[3];
    float e = M[0] * T[4] + M[2] * T[5] + M[4];
    float f = M[1] * T[4] + M[3] * T[5] + M[5];
    M[0] = a; M[1] = b; M[2] = c; M[3] = d; M[4] = e; M[5] = f;
}

std::vector<float> READSVG::ParseTransformMatrix(const std::string& s) {
    std::string t = _trim(s);
    if (t.empty()) return {};
    float M[6] = { 1,0,0,1, 0,0 };
    size_t i = 0, n = t.size();
    auto isAlpha = [&](char c) { return std::isalpha((unsigned char)c) != 0; };
    while (i < n) {
        while (i < n && std::isspace((unsigned char)t[i])) ++i;
        if (i >= n) break;
        size_t j = i;
        while (j < n&& isAlpha(t[j])) ++j;
        std::string fn = t.substr(i, j - i);
        while (j < n&& std::isspace((unsigned char)t[j])) ++j;
        if (j >= n || t[j] != '(') break;
        ++j; // '('
        size_t k = j; int paren = 1;
        while (k < n && paren>0) { if (t[k] == '(') ++paren; else if (t[k] == ')') --paren; ++k; }
        if (paren != 0) break;
        std::string args = t.substr(j, (k - 1) - j);
        i = k;

        auto vals = _splitFloatList(args);
        float Tm[6] = { 1,0,0,1, 0,0 };

        if (_ieq(fn, "matrix") && vals.size() >= 6) {
            Tm[0] = vals[0]; Tm[1] = vals[1];
            Tm[2] = vals[2]; Tm[3] = vals[3];
            Tm[4] = vals[4]; Tm[5] = vals[5];
        }
        else if (_ieq(fn, "translate")) {
            float tx = vals.size() > 0 ? vals[0] : 0.f;
            float ty = vals.size() > 1 ? vals[1] : 0.f;
            Tm[4] = tx; Tm[5] = ty;
        }
        else if (_ieq(fn, "scale")) {
            float sx = vals.size() > 0 ? vals[0] : 1.f;
            float sy = vals.size() > 1 ? vals[1] : sx;
            Tm[0] = sx; Tm[3] = sy;
        }
        else if (_ieq(fn, "rotate")) {
            float a = vals.size() > 0 ? vals[0] : 0.f; // độ
            float rad = a * 3.14159265358979323846f / 180.f;
            float ca = std::cos(rad), sa = std::sin(rad);
            Tm[0] = ca; Tm[1] = sa; Tm[2] = -sa; Tm[3] = ca;
        }
        else if (_ieq(fn, "skewX")) {
            float a = vals.size() > 0 ? vals[0] : 0.f;
            float rad = a * 3.14159265358979323846f / 180.f;
            Tm[2] = std::tan(rad);
        }
        else if (_ieq(fn, "skewY")) {
            float a = vals.size() > 0 ? vals[0] : 0.f;
            float rad = a * 3.14159265358979323846f / 180.f;
            Tm[1] = std::tan(rad);
        }
        else {
            continue;
        }
        _matMul(M, Tm);
    }
    return { M[0],M[1],M[2],M[3],M[4],M[5] };
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
    auto v = _getAttrWithStyle(_node[index], "fill-rule");
    return v.empty() ? "nonzero" : v;
}
std::string READSVG::GetStrokeLinecap(int index) {
    auto v = _getAttrWithStyle(_node[index], "stroke-linecap");
    return v.empty() ? "butt" : v;
}
std::string READSVG::GetStrokeLinejoin(int index) {
    auto v = _getAttrWithStyle(_node[index], "stroke-linejoin");
    return v.empty() ? "miter" : v;
}
float READSVG::GetStrokeMiterlimit(int index) {
    auto v = _getAttrWithStyle(_node[index], "stroke-miterlimit");
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
std::vector<float> READSVG::GetTransformMatrix(int index) {
    auto v = _getAttrWithStyle(_node[index], "transform");
    return ParseTransformMatrix(v);
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
bool READSVG::HasViewBox(int index) {
    auto it = _node[index]._att.find("viewBox");
    return it != _node[index]._att.end() && !_trim(it->second).empty();
}
std::tuple<float, float, float, float> READSVG::GetViewBox(int index) {
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
    auto v = _getAttrWithStyle(_node[index], "font-family");
    return v.empty() ? "Times New Roman" : v;
}
float READSVG::GetFontSize(int index) {
    auto v = _getAttrWithStyle(_node[index], "font-size");
    return ParseFloat(v, 16.0f);
}
std::string READSVG::GetFontWeight(int index) {
    auto v = _getAttrWithStyle(_node[index], "font-weight");
    return v.empty() ? "normal" : v;
}
std::string READSVG::GetFontStyle(int index) {
    auto v = _getAttrWithStyle(_node[index], "font-style");
    return v.empty() ? "normal" : v;
}
std::string READSVG::GetTextAnchor(int index) {
    auto v = _getAttrWithStyle(_node[index], "text-anchor");
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
    std::string v = FindInStyle(i, "fill:");
    if (v.empty()) v = GetAttrRaw(i, "fill");
    // so sánh lower-case
    for (auto& c : v) c = (char)tolower((unsigned char)c);
    return v == "none";
}