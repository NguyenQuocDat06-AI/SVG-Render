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