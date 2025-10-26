#ifndef READSVG_H
#define READSVG_H
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

#include "rapidxml.hpp"
// #include "rapidxml_utils.hpp" // Nếu muốn dùng file<>

using namespace rapidxml;
using namespace std;
struct ATTRIBUTE
{
	string _tags = "";
	string _text = "";
	map<string, string> _att;
};
class READSVG {
private:
	vector<ATTRIBUTE> _node;
	vector<char> ReadFileToBuffer(const string& _path); // Đọc từ file svg
public:
	READSVG();
	void ParseFromBuffer(const string& _path); // Lưu vào các node
	void PrintNode();
	~READSVG();
};
#endif // !READSVG_H
