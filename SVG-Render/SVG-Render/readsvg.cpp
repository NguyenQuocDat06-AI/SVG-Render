#include "readsvg.h"
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
void READSVG::ParseFromBuffer(const string& _path){
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
vector<int> READSVG::getColor(const string& str) {
	vector<int> rgb(3, 0);
    if (str[0] == '#') {
        if (str.length() == 7) { // #RRGGBB
            rgb[0] = stoi(str.substr(1, 2), nullptr, 16); // R
            rgb[1] = stoi(str.substr(3, 2), nullptr, 16); // G
            rgb[2] = stoi(str.substr(5, 2), nullptr, 16); // B
        }
        else if (str.length() == 4) { // #RGB
            string r(2, str[1]);
            string g(2, str[2]);
            string b(2, str[3]);
            rgb[0] = stoi(r, nullptr, 16); // R
            rgb[1] = stoi(g, nullptr, 16); // G
            rgb[2] = stoi(b, nullptr, 16); // B
        }
    }
    else if (str.find("rgb(") == 0 && str.back() == ')') {
        string content = str.substr(4, str.length() - 5); // Lấy nội dung bên trong rgb(...)
        size_t pos = 0;
        for (int i = 0; i < 3; i++) {
            size_t commaPos = content.find(',', pos);
            string valueStr = (commaPos == string::npos) ? content.substr(pos) : content.substr(pos, commaPos - pos);
            rgb[i] = stoi(valueStr);
            pos = commaPos + 1;
        }
    }
    return rgb;
}
ATTRIBUTE READSVG::GetNode(int index) {
    if (index < 0 || index >= _node.size()) {
        throw out_of_range("Index out of range in GetNode");
    }
    return _node[index];
}
vector<int> READSVG::GetFill(int index) {
	string str = _node[index]._att["fill"];
    vector<int> rgb(3, 0);
    return getColor(str);
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
vector<int> READSVG::GetStroke(int index) {
    string str = _node[index]._att["stroke"];
    vector<int> rgb(3, 0);
    return getColor(str);
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
vector<pair<float,float>> READSVG::GetPoints(int index) {
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