#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include "path.h"
// Nhớ include header của class SVGPATH và GDI+

// Hàm hỗ trợ xử lý chuỗi (chuyển dấu phẩy thành khoảng trắng)

SVGPATH::SVGPATH(string d)
    : d(d) {
}

void SVGPATH::SetLinearGradient(const Gdiplus::PointF& p1, const Gdiplus::PointF& p2,
    const std::vector<Gdiplus::Color>& colors,
    const std::vector<float>& offsets) {
    if (colors.empty() || offsets.empty() || colors.size() != offsets.size()) {
        useLinearGradient = false;
        gColors.clear();
        gOffsets.clear();
        return;
    }
    useLinearGradient = true;
    gStart = p1;
    gEnd = p2;
    gColors = colors;
    gOffsets = offsets;
}

// Hàm helper để thay thế dấu phẩy bằng khoảng trắng (nếu bạn chưa có)
std::string FormatPathString(std::string str) {
    for (char &c : str) {
        if (c == ',') c = ' ';
    }
    return str;
}

inline bool IsSeparator(char c) {
    return (c == ',' || std::isspace(static_cast<unsigned char>(c)));
}

// Helper: Đọc 1 số thực từ stringstream, xử lý cả trường hợp dính liền (vd: 12-34 -> 12, -34)
bool ReadFloat(std::stringstream& ss, float& outVal) {
    // Bỏ qua separator
    while (ss.good() && IsSeparator(ss.peek())) {
        ss.get();
    }
    if (ss.eof()) return false;

    // Kiểm tra xem có phải bắt đầu số không
    char next = ss.peek();
    if (!isdigit(static_cast<unsigned char>(next)) && next != '-' && next != '+' && next != '.') {
        return false;
    }

    ss >> outVal;
    return !ss.fail();
}

void SVGPATH::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
    if (d.empty()) return;

    Gdiplus::GraphicsPath path;

    // 1. Setup Fill Mode
    if (!fillRule.empty()) {
        std::string ruleLower = fillRule;
        std::transform(ruleLower.begin(), ruleLower.end(), ruleLower.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (ruleLower == "evenodd") path.SetFillMode(Gdiplus::FillModeAlternate);
        else path.SetFillMode(Gdiplus::FillModeWinding);
    }

    // 2. Parsing Path Data
    std::stringstream ss(d);
    char cmd = 0;
    float args[7]; // Đủ chứa tham số cho lệnh C/c (6 số)
    Gdiplus::PointF curr(0, 0);     // Điểm hiện tại
    Gdiplus::PointF startFig(0, 0); // Điểm bắt đầu của sub-path (để đóng Z)

    // Last control point for smooth curves (S/s, T/t) - chưa support S/T ở đây nhưng cần logic
    // Gdiplus::PointF lastCtrl = curr; 

    while (true) {
        // Bỏ qua separator
        while (ss.good() && IsSeparator(ss.peek())) ss.get();
        if (ss.eof()) break;

        char next = ss.peek();

        // Nếu là chữ cái -> Lệnh mới
        if (std::isalpha(static_cast<unsigned char>(next))) {
            ss >> cmd;
        }
        // Nếu là số/dấu -> Lặp lại lệnh cũ (Implicit command)
        else if (cmd == 0) {
            ss.get(); continue; // Bỏ qua ký tự lạ đầu chuỗi
        }
        // Đặc biệt: Sau M/m, các số tiếp theo được hiểu là L/l
        else if (cmd == 'M') cmd = 'L';
        else if (cmd == 'm') cmd = 'l';


        // Xử lý từng lệnh
        switch (cmd) {
            // --- MoveTo ---
        case 'M': // Absolute
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1])) {
                path.StartFigure();
                curr = Gdiplus::PointF(args[0], args[1]);
                startFig = curr;
            }
            break;
        case 'm': // Relative
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1])) {
                path.StartFigure();
                curr = Gdiplus::PointF(curr.X + args[0], curr.Y + args[1]);
                startFig = curr;
            }
            break;

            // --- LineTo ---
        case 'L':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1])) {
                Gdiplus::PointF p(args[0], args[1]);
                path.AddLine(curr, p);
                curr = p;
            }
            break;
        case 'l':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1])) {
                Gdiplus::PointF p(curr.X + args[0], curr.Y + args[1]);
                path.AddLine(curr, p);
                curr = p;
            }
            break;

            // --- Horizontal LineTo ---
        case 'H':
            if (ReadFloat(ss, args[0])) {
                Gdiplus::PointF p(args[0], curr.Y);
                path.AddLine(curr, p);
                curr = p;
            }
            break;
        case 'h':
            if (ReadFloat(ss, args[0])) {
                Gdiplus::PointF p(curr.X + args[0], curr.Y);
                path.AddLine(curr, p);
                curr = p;
            }
            break;

            // --- Vertical LineTo ---
        case 'V':
            if (ReadFloat(ss, args[0])) {
                Gdiplus::PointF p(curr.X, args[0]);
                path.AddLine(curr, p);
                curr = p;
            }
            break;
        case 'v':
            if (ReadFloat(ss, args[0])) {
                Gdiplus::PointF p(curr.X, curr.Y + args[0]);
                path.AddLine(curr, p);
                curr = p;
            }
            break;

            // --- Cubic Bezier (C x1 y1 x2 y2 x y) ---
        case 'C':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1]) &&
                ReadFloat(ss, args[2]) && ReadFloat(ss, args[3]) &&
                ReadFloat(ss, args[4]) && ReadFloat(ss, args[5])) {
                path.AddBezier(curr,
                    Gdiplus::PointF(args[0], args[1]),
                    Gdiplus::PointF(args[2], args[3]),
                    Gdiplus::PointF(args[4], args[5]));
                curr = Gdiplus::PointF(args[4], args[5]);
            }
            break;
        case 'c':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1]) &&
                ReadFloat(ss, args[2]) && ReadFloat(ss, args[3]) &&
                ReadFloat(ss, args[4]) && ReadFloat(ss, args[5])) {
                path.AddBezier(curr,
                    Gdiplus::PointF(curr.X + args[0], curr.Y + args[1]),
                    Gdiplus::PointF(curr.X + args[2], curr.Y + args[3]),
                    Gdiplus::PointF(curr.X + args[4], curr.Y + args[5]));
                curr = Gdiplus::PointF(curr.X + args[4], curr.Y + args[5]);
            }
            break;

            // --- ClosePath ---
        case 'Z':
        case 'z':
            path.CloseFigure();
            curr = startFig;
            // Sau Z không có tham số, reset implicit command
            // Nhưng theo chuẩn, nếu có Z thì lệnh tiếp theo bắt buộc phải là M/m hoặc kết thúc
            break;

        default:
            // Lệnh chưa hỗ trợ (S, Q, T, A...) -> Bỏ qua ký tự này để tránh loop vô hạn
            // ss.get(); 
            // Tốt nhất là break vòng lặp nếu gặp lệnh lạ để tránh treo
            return;
        }
    }

    // 3. Render (Phần này giữ nguyên logic cũ của bạn)
    if (hasFill) {
        if (useLinearGradient && !gColors.empty() && !gOffsets.empty() && gColors.size() == gOffsets.size()) {
            std::vector<Gdiplus::Color> cols = gColors;
            for (auto& c : cols) c = Gdiplus::Color(finalFillAlpha, c.GetR(), c.GetG(), c.GetB());

            Gdiplus::LinearGradientBrush brush(gStart, gEnd, cols.front(), cols.back());
            if (cols.size() > 2) {
                brush.SetInterpolationColors(cols.data(), gOffsets.data(), static_cast<INT>(cols.size()));
            }
            // Áp dụng Transform cho gradient nếu cần (như bài trước)
            if (hasTransform) {
                brush.MultiplyTransform(&transform, Gdiplus::MatrixOrderPrepend);
            }
            g.FillPath(&brush, &path);
        }
        else {
            Gdiplus::SolidBrush fillBrush(Gdiplus::Color(finalFillAlpha, fillColor.GetR(), fillColor.GetG(), fillColor.GetB()));
            g.FillPath(&fillBrush, &path);
        }
    }

    if (hasStroke && strokeWidth > 0) {
        Gdiplus::Pen strokePen(Gdiplus::Color(finalStrokeAlpha, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB()), strokeWidth);
        strokePen.SetMiterLimit(strokeMiterLimit);

        std::string joinLower = strokeLinejoin;
        std::transform(joinLower.begin(), joinLower.end(), joinLower.begin(), [](unsigned char c) { return std::tolower(c); });

        if (joinLower == "round") strokePen.SetLineJoin(Gdiplus::LineJoinRound);
        else if (joinLower == "bevel") strokePen.SetLineJoin(Gdiplus::LineJoinBevel);
        else strokePen.SetLineJoin(Gdiplus::LineJoinMiter);

        g.DrawPath(&strokePen, &path);
    }
}