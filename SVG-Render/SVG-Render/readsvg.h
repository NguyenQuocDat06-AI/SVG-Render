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
	vector<int> getColor(const string& str);
	ATTRIBUTE GetNode(int index);
	vector<int> GetFill(int index);
	float GetFillOpacity(int index);
	float GetHeight(int index);
	float GetWidth(int index);
	vector<int> GetStroke(int index);
	float GetStrokeOpacity(int index);
	float GetStrokeWidth(int index);
	float GetX(int index);
	float GetY(int index);
	float GetCx(int index);
	float GetCy(int index);
	float GetRx(int index);
	float GetRy(int index);
	float GetR(int index);
	vector<pair<float, float>> GetPoints(int index);
	pair<float, float> GetSize(int index);
	~READSVG();
};
#endif // !READSVG_H
