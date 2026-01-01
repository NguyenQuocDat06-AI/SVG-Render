#include "path.h"
#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
// =========================================================
// CÁC HÀM HELPER BỔ TRỢ
// =========================================================

// Kiểm tra ký tự phân tách (dấu phẩy hoặc khoảng trắng)
inline bool IsSeparator(char c) {
    return (c == ',' || std::isspace(static_cast<unsigned char>(c)));
}

// Đọc số thực từ stream, xử lý trường hợp dính liền (vd: 10-20)
bool ReadFloat(std::stringstream& ss, float& outVal) {
    // Bỏ qua separator
    while (ss.good() && IsSeparator(ss.peek())) {
        ss.get();
    }
    if (ss.eof()) return false;

    // Kiểm tra ký tự đầu tiên của số
    char next = ss.peek();
    if (!isdigit(static_cast<unsigned char>(next)) && next != '-' && next != '+' && next != '.') {
        return false;
    }

    ss >> outVal;
    return !ss.fail();
}

// Tính điểm đối xứng của p qua center (Dùng cho lệnh S/s)
Gdiplus::PointF Reflect(const Gdiplus::PointF& p, const Gdiplus::PointF& center) {
    return Gdiplus::PointF(2 * center.X - p.X, 2 * center.Y - p.Y);
}

// Hàm format chuỗi cũ (giữ lại nếu cần, nhưng ReadFloat đã xử lý tốt hơn)
std::string FormatPathString(std::string str) {
    for (char& c : str) {
        if (c == ',') c = ' ';
    }
    return str;
}

// =========================================================
// SVGPATH CLASS IMPLEMENTATION
// =========================================================

SVGPATH::SVGPATH(std::string d) : d(d) {
}

void SVGPATH::SetLinearGradient(const Gdiplus::PointF& p1, const Gdiplus::PointF& p2,
    const std::vector<Gdiplus::Color>& colors,
    const std::vector<float>& offsets,
    const std::vector<float>& transformMtx) // <---
{
    // ... (Code cũ check rỗng) ...
    useLinearGradient = true;
    gStart = p1; gEnd = p2; gColors = colors; gOffsets = offsets;

    // Lưu transform
    if (transformMtx.size() >= 6) {
        gradMatrix.SetElements(transformMtx[0], transformMtx[1], transformMtx[2],
            transformMtx[3], transformMtx[4], transformMtx[5]);
        hasGradTransform = true;
    }
    else {
        gradMatrix.Reset();
        hasGradTransform = false;
    }
}


void SVGPATH::SetRadialGradient(float cx, float cy, float r, float fx, float fy,
    bool userSpace,
    const std::vector<Gdiplus::Color>& colors,
    const std::vector<float>& offsets,
    const std::vector<float>& transformMtx) // <---
{
    // ... (Code cũ check rỗng) ...
    useRadialGradient = true; useLinearGradient = false;
    radCx = cx; radCy = cy; radR = r; radFx = fx; radFy = fy;
    radUserSpace = userSpace; radColors = colors; radOffsets = offsets;

    // Lưu transform
    if (transformMtx.size() >= 6) {
        gradMatrix.SetElements(transformMtx[0], transformMtx[1], transformMtx[2],
            transformMtx[3], transformMtx[4], transformMtx[5]);
        hasGradTransform = true;
    }
    else {
        gradMatrix.Reset();
        hasGradTransform = false;
    }
}



// Hàm helper để thêm Arc vào Path bằng cách xấp xỉ Bezier
void AddSvgArcToPath(Gdiplus::GraphicsPath& path,
    Gdiplus::PointF& cursor,
    float rx, float ry,
    float angle,
    bool largeArcFlag, bool sweepFlag,
    float x, float y) {

    // 1. Xử lý các trường hợp đặc biệt
    if (rx == 0 || ry == 0) {
        // Nếu bán kính = 0, Arc suy biến thành đường thẳng
        path.AddLine(cursor, Gdiplus::PointF(x, y));
        cursor = Gdiplus::PointF(x, y);
        return;
    }

    // Lấy trị tuyệt đối của bán kính
    rx = std::abs(rx);
    ry = std::abs(ry);

    // Điểm hiện tại (x0, y0) và điểm đích (x, y)
    float x0 = cursor.X;
    float y0 = cursor.Y;

    // Nếu điểm đầu trùng điểm cuối, bỏ qua
    if (x0 == x && y0 == y) return;

    // 2. Tính toán theo công thức SVG Implementation Notes (F.6.5)
    // Chuyển đổi từ độ sang radian
    double phi = angle * M_PI / 180.0;
    double sinPhi = std::sin(phi);
    double cosPhi = std::cos(phi);

    // Bước 1: Tính toạ độ điểm P1' (trong hệ trục đã xoay)
    double dx = (x0 - x) / 2.0;
    double dy = (y0 - y) / 2.0;
    double x1p = cosPhi * dx + sinPhi * dy;
    double y1p = -sinPhi * dx + cosPhi * dy;

    // Bước 2: Điều chỉnh bán kính nếu cần
    double rx_sq = rx * rx;
    double ry_sq = ry * ry;
    double x1p_sq = x1p * x1p;
    double y1p_sq = y1p * y1p;

    // Kiểm tra xem bán kính có đủ lớn để nối 2 điểm không
    double lambda = x1p_sq / rx_sq + y1p_sq / ry_sq;
    if (lambda > 1.0) {
        // Scale bán kính lên
        double lambdaRoot = std::sqrt(lambda);
        rx *= lambdaRoot;
        ry *= lambdaRoot;
        rx_sq = rx * rx;
        ry_sq = ry * ry;
    }

    // Bước 3: Tính tâm C' (cx', cy')
    double sign = (largeArcFlag == sweepFlag) ? -1.0 : 1.0;
    double numerator = rx_sq * ry_sq - rx_sq * y1p_sq - ry_sq * x1p_sq;
    // Tránh lỗi chia cho 0 hoặc căn số âm do sai số float
    if (numerator < 0.0) numerator = 0.0;
    double denominator = rx_sq * y1p_sq + ry_sq * x1p_sq;

    double coef = sign * std::sqrt(numerator / denominator);
    double cxp = coef * ((rx * y1p) / ry);
    double cyp = coef * (-(ry * x1p) / rx);

    // Bước 4: Tính tâm C thực tế (cx, cy)
    double cx = cosPhi * cxp - sinPhi * cyp + (x0 + x) / 2.0;
    double cy = sinPhi * cxp + cosPhi * cyp + (y0 + y) / 2.0;

    // Bước 5: Tính các góc bắt đầu và góc quét (Start Angle & Delta Angle)
    auto angleBetween = [](double ux, double uy, double vx, double vy) {
        double sign = (ux * vy - uy * vx < 0) ? -1.0 : 1.0;
        double dot = ux * vx + uy * vy;
        double lenU = std::sqrt(ux * ux + uy * uy);
        double lenV = std::sqrt(vx * vx + vy * vy);
        double arg = dot / (lenU * lenV);
        if (arg > 1.0) arg = 1.0;
        else if (arg < -1.0) arg = -1.0;
        return sign * std::acos(arg);
        };

    // Vector (1, 0)
    double startAngle = angleBetween(1.0, 0.0, (x1p - cxp) / rx, (y1p - cyp) / ry);
    double deltaAngle = angleBetween((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry);

    // Xử lý sweep flag
    if (!sweepFlag && deltaAngle > 0) deltaAngle -= 2.0 * M_PI;
    else if (sweepFlag && deltaAngle < 0) deltaAngle += 2.0 * M_PI;

    // Bước 6: Chia cung thành các đoạn nhỏ và vẽ bằng Bezier
    // Mỗi đoạn Bezier không nên vượt quá 90 độ (PI/2) để đảm bảo độ chính xác
    int segments = static_cast<int>(std::ceil(std::abs(deltaAngle) / (M_PI / 2.0)));
    double theta = startAngle;
    double thetaStep = deltaAngle / segments;
    double t = (8.0 / 3.0) * std::sin(thetaStep / 4.0) * std::sin(thetaStep / 4.0) / std::sin(thetaStep / 2.0);

    for (int i = 0; i < segments; ++i) {
        double cosTheta = std::cos(theta);
        double sinTheta = std::sin(theta);
        double thetaNext = theta + thetaStep;
        double cosThetaNext = std::cos(thetaNext);
        double sinThetaNext = std::sin(thetaNext);

        // Điểm điều khiển trên đơn vị tròn
        double e1x = cosTheta - t * sinTheta;
        double e1y = sinTheta + t * cosTheta;
        double e2x = cosThetaNext + t * sinThetaNext;
        double e2y = sinThetaNext - t * cosThetaNext;

        // Hàm transform điểm từ đơn vị tròn sang elip xoay
        auto transformPoint = [&](double px, double py) {
            double tx = rx * px;
            double ty = ry * py;
            double finalX = cosPhi * tx - sinPhi * ty + cx;
            double finalY = sinPhi * tx + cosPhi * ty + cy;
            return Gdiplus::PointF((float)finalX, (float)finalY);
            };

        Gdiplus::PointF cp1 = transformPoint(e1x, e1y);
        Gdiplus::PointF cp2 = transformPoint(e2x, e2y);
        Gdiplus::PointF endPt = transformPoint(cosThetaNext, sinThetaNext);

        // Thêm curve vào path
        path.AddBezier(cursor, cp1, cp2, endPt);

        // Cập nhật điểm hiện tại
        cursor = endPt;
        theta = thetaNext;
    }
}

bool ReadFlag(std::stringstream& ss, bool& outVal) {
    // Bỏ qua separator (khoảng trắng, phẩy)
    while (ss.good() && IsSeparator(ss.peek())) {
        ss.get();
    }
    if (ss.eof()) return false;

    char c = ss.peek();
    if (c == '0') {
        outVal = false;
        ss.get(); // Ăn ký tự '0'
        return true;
    }
    else if (c == '1') {
        outVal = true;
        ss.get(); // Ăn ký tự '1'
        return true;
    }
    return false; // Không phải 0/1 là lỗi
}


void SVGPATH::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
    if (d.empty()) return;

    // Transform của shape đã được áp dụng trong `SVGSHAPE::Draw`
    // nên ở đây không nhân thêm nữa, tránh việc gradient bị lệch
    // khi đứng yên và chỉ đúng màu khi pan/zoom.

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
    Gdiplus::PointF startFig(0, 0); // Điểm bắt đầu sub-path

    // Biến hỗ trợ đường cong trơn (Smooth Curve S/s, T/t)
    Gdiplus::PointF lastCtrl = curr;
    char lastCmd = 0;

    while (true) {
        // Bỏ qua separator
        while (ss.good() && IsSeparator(ss.peek())) ss.get();
        if (ss.eof()) break;

        char next = ss.peek();

        // Nếu là chữ cái -> Lệnh mới
        if (std::isalpha(static_cast<unsigned char>(next))) {
            ss >> cmd;
        }
        // Nếu là số/dấu -> Lệnh cũ (Implicit)
        else if (cmd == 0) {
            ss.get(); continue;
        }
        // Implicit command sau M/m là L/l
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
                lastCtrl = curr; // Reset control point
            }
            break;
        case 'm': // Relative
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1])) {
                path.StartFigure();
                curr = Gdiplus::PointF(curr.X + args[0], curr.Y + args[1]);
                startFig = curr;
                lastCtrl = curr;
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

                Gdiplus::PointF p1(args[0], args[1]);
                Gdiplus::PointF p2(args[2], args[3]);
                Gdiplus::PointF p3(args[4], args[5]);

                path.AddBezier(curr, p1, p2, p3);

                curr = p3;
                lastCtrl = p2; // Lưu điểm điều khiển 2
            }
            break;
        case 'c':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1]) &&
                ReadFloat(ss, args[2]) && ReadFloat(ss, args[3]) &&
                ReadFloat(ss, args[4]) && ReadFloat(ss, args[5])) {

                Gdiplus::PointF p1(curr.X + args[0], curr.Y + args[1]);
                Gdiplus::PointF p2(curr.X + args[2], curr.Y + args[3]);
                Gdiplus::PointF p3(curr.X + args[4], curr.Y + args[5]);

                path.AddBezier(curr, p1, p2, p3);

                curr = p3;
                lastCtrl = p2;
            }
            break;

            // --- Smooth Cubic Bezier (S x2 y2 x y) ---
            // Điểm điều khiển 1 là đối xứng của lastCtrl qua curr
        case 'S':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1]) &&
                ReadFloat(ss, args[2]) && ReadFloat(ss, args[3])) {

                Gdiplus::PointF p2(args[0], args[1]);
                Gdiplus::PointF p3(args[2], args[3]);

                // Tính p1 tự động
                Gdiplus::PointF p1 = curr;
                if (lastCmd == 'C' || lastCmd == 'c' || lastCmd == 'S' || lastCmd == 's') {
                    p1 = Reflect(lastCtrl, curr);
                }

                path.AddBezier(curr, p1, p2, p3);

                curr = p3;
                lastCtrl = p2;
            }
            break;
        case 's':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1]) &&
                ReadFloat(ss, args[2]) && ReadFloat(ss, args[3])) {

                Gdiplus::PointF p2(curr.X + args[0], curr.Y + args[1]);
                Gdiplus::PointF p3(curr.X + args[2], curr.Y + args[3]);

                // Tính p1 tự động
                Gdiplus::PointF p1 = curr;
                if (lastCmd == 'C' || lastCmd == 'c' || lastCmd == 'S' || lastCmd == 's') {
                    p1 = Reflect(lastCtrl, curr);
                }

                path.AddBezier(curr, p1, p2, p3);

                curr = p3;
                lastCtrl = p2;
            }
            break;

            // --- ClosePath ---
        case 'Z':
        case 'z':
            path.CloseFigure();
            curr = startFig;
            break;
            // --- Quadratic Bezier (Q x1 y1 x y) ---
                // GDI+ không hỗ trợ Quadratic, phải convert sang Cubic
        case 'Q':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1]) &&
                ReadFloat(ss, args[2]) && ReadFloat(ss, args[3])) {

                Gdiplus::PointF qCtrl(args[0], args[1]); // Điểm điều khiển Quadratic
                Gdiplus::PointF end(args[2], args[3]);   // Điểm cuối

                // Tính toán 2 điểm điều khiển cho Cubic Bezier (công thức 2/3)
                Gdiplus::PointF c1(
                    curr.X + (2.0f / 3.0f) * (qCtrl.X - curr.X),
                    curr.Y + (2.0f / 3.0f) * (qCtrl.Y - curr.Y)
                );
                Gdiplus::PointF c2(
                    end.X + (2.0f / 3.0f) * (qCtrl.X - end.X),
                    end.Y + (2.0f / 3.0f) * (qCtrl.Y - end.Y)
                );

                path.AddBezier(curr, c1, c2, end);

                curr = end;
                lastCtrl = qCtrl; // Lưu điểm điều khiển gốc để dùng cho lệnh T tiếp theo (nếu có)
            }
            break;

        case 'q':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1]) &&
                ReadFloat(ss, args[2]) && ReadFloat(ss, args[3])) {

                Gdiplus::PointF qCtrl(curr.X + args[0], curr.Y + args[1]);
                Gdiplus::PointF end(curr.X + args[2], curr.Y + args[3]);

                // Convert sang Cubic
                Gdiplus::PointF c1(
                    curr.X + (2.0f / 3.0f) * (qCtrl.X - curr.X),
                    curr.Y + (2.0f / 3.0f) * (qCtrl.Y - curr.Y)
                );
                Gdiplus::PointF c2(
                    end.X + (2.0f / 3.0f) * (qCtrl.X - end.X),
                    end.Y + (2.0f / 3.0f) * (qCtrl.Y - end.Y)
                );

                path.AddBezier(curr, c1, c2, end);

                curr = end;
                lastCtrl = qCtrl;
            }
            break;

            // --- Smooth Quadratic Bezier (T x y) ---
            // Điểm điều khiển được suy ra từ điểm điều khiển của lệnh trước đó
        case 'T':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1])) {
                Gdiplus::PointF end(args[0], args[1]);

                // Tính điểm điều khiển tự động (đối xứng qua curr)
                Gdiplus::PointF qCtrl = curr;
                if (lastCmd == 'Q' || lastCmd == 'q' || lastCmd == 'T' || lastCmd == 't') {
                    qCtrl = Reflect(lastCtrl, curr);
                }

                // Convert sang Cubic
                Gdiplus::PointF c1(
                    curr.X + (2.0f / 3.0f) * (qCtrl.X - curr.X),
                    curr.Y + (2.0f / 3.0f) * (qCtrl.Y - curr.Y)
                );
                Gdiplus::PointF c2(
                    end.X + (2.0f / 3.0f) * (qCtrl.X - end.X),
                    end.Y + (2.0f / 3.0f) * (qCtrl.Y - end.Y)
                );

                path.AddBezier(curr, c1, c2, end);

                curr = end;
                lastCtrl = qCtrl;
            }
            break;

        case 't':
            if (ReadFloat(ss, args[0]) && ReadFloat(ss, args[1])) {
                Gdiplus::PointF end(curr.X + args[0], curr.Y + args[1]);

                Gdiplus::PointF qCtrl = curr;
                if (lastCmd == 'Q' || lastCmd == 'q' || lastCmd == 'T' || lastCmd == 't') {
                    qCtrl = Reflect(lastCtrl, curr);
                }

                Gdiplus::PointF c1(
                    curr.X + (2.0f / 3.0f) * (qCtrl.X - curr.X),
                    curr.Y + (2.0f / 3.0f) * (qCtrl.Y - curr.Y)
                );
                Gdiplus::PointF c2(
                    end.X + (2.0f / 3.0f) * (qCtrl.X - end.X),
                    end.Y + (2.0f / 3.0f) * (qCtrl.Y - end.Y)
                );

                path.AddBezier(curr, c1, c2, end);

                curr = end;
                lastCtrl = qCtrl;
            }
            break;
            // --- Elliptical Arc (A rx ry rot large sweep x y) ---
        case 'A':
        {
            // Khai báo biến tạm để đọc
            float rx, ry, rot, x, y;
            bool large, sweep;

            // 3 tham số đầu là float
            if (ReadFloat(ss, rx) && ReadFloat(ss, ry) && ReadFloat(ss, rot)) {

                // QUAN TRỌNG: 2 tham số tiếp theo là Flag -> Dùng ReadFlag
                if (ReadFlag(ss, large) && ReadFlag(ss, sweep)) {

                    // 2 tham số cuối là tọa độ -> Dùng ReadFloat
                    if (ReadFloat(ss, x) && ReadFloat(ss, y)) {

                        AddSvgArcToPath(path, curr, rx, ry, rot, large, sweep, x, y);

                        curr = Gdiplus::PointF(x, y);
                        lastCtrl = curr;
                    }
                }
            }
            break;
        }

        case 'a':
        {
            float rx, ry, rot, dx, dy;
            bool large, sweep;

            if (ReadFloat(ss, rx) && ReadFloat(ss, ry) && ReadFloat(ss, rot)) {

                // QUAN TRỌNG: Dùng ReadFlag
                if (ReadFlag(ss, large) && ReadFlag(ss, sweep)) {

                    if (ReadFloat(ss, dx) && ReadFloat(ss, dy)) {

                        float x = curr.X + dx;
                        float y = curr.Y + dy;

                        AddSvgArcToPath(path, curr, rx, ry, rot, large, sweep, x, y);

                        curr = Gdiplus::PointF(x, y);
                        lastCtrl = curr;
                    }
                }
            }
            break;
        }
        default:
            return; // Gặp lệnh lạ thì thoát
        }

        lastCmd = cmd; // Cập nhật lệnh vừa thực thi
    }

    // 3. Render
    if (hasFill) {
        if (useLinearGradient && !gColors.empty()) {
            // 1. Lấy khung bao chính xác của Path
            Gdiplus::RectF bounds;
            path.GetBounds(&bounds);

            // Tránh lỗi nếu Path là đường thẳng (Width hoặc Height = 0)
            if (bounds.Width == 0) bounds.Width = 1.0f;
            if (bounds.Height == 0) bounds.Height = 1.0f;

            // 2. Chuyển đổi tọa độ từ đơn vị tỉ lệ (0..1) sang Pixel tuyệt đối
            // p1, p2 được tính dựa trên khung bao của hình
            Gdiplus::PointF p1(
                bounds.X + gStart.X * bounds.Width,
                bounds.Y + gStart.Y * bounds.Height
            );
            Gdiplus::PointF p2(
                bounds.X + gEnd.X * bounds.Width,
                bounds.Y + gEnd.Y * bounds.Height
            );

            // Xử lý các điểm dừng (Stops) và Độ trong suốt (Stop-Opacity)
            // Giả sử gColors đã chứa Alpha từ stop-opacity lúc parse
            std::vector<Gdiplus::Color> finalCols;
            for (const auto& c : gColors) {
                // Lấy alpha gốc của stop (c.GetA()) nhân với alpha tổng thể (finalFillAlpha)
                BYTE combinedAlpha = (BYTE)((c.GetAlpha() * finalFillAlpha) / 255);
                finalCols.emplace_back(combinedAlpha, c.GetR(), c.GetG(), c.GetB());
            }

            // GDI+ yêu cầu LinearGradientBrush cần ít nhất 2 màu để khởi tạo
            Gdiplus::LinearGradientBrush brush(p1, p2, finalCols.front(), finalCols.back());

            // 3. Thiết lập mảng màu Interpolation (Quan trọng để hiển thị đúng dải màu SVG)
            if (finalCols.size() == gOffsets.size()) {
                std::vector<float> finalOffs = gOffsets;

                // GDI+ yêu cầu offset đầu là 0.0 và cuối là 1.0
                if (finalOffs.front() > 0.001f) {
                    finalOffs.insert(finalOffs.begin(), 0.0f);
                    finalCols.insert(finalCols.begin(), finalCols.front());
                }
                if (finalOffs.back() < 0.999f) {
                    finalOffs.push_back(1.0f);
                    finalCols.push_back(finalCols.back());
                }

                brush.SetInterpolationColors(finalCols.data(), finalOffs.data(), (INT)finalCols.size());
            }

            // 4. Xử lý GradientTransform (Nếu có)
            // Với objectBoundingBox, transform diễn ra trong không gian đơn vị trước khi map vào bounds
            if (hasGradTransform) {
                // Dịch chuyển về gốc tọa độ của hình để xoay/scale đúng tâm, sau đó mới dịch trở lại
                Gdiplus::Matrix m;
                m.Translate(bounds.X, bounds.Y);
                m.Scale(bounds.Width, bounds.Height);

                // Nhân ma trận biến đổi của Gradient
                m.Multiply(&gradMatrix, Gdiplus::MatrixOrderPrepend);

                // Đảo ngược lại ma trận Mapping ban đầu để brush áp dụng transform chính xác
                Gdiplus::Matrix invM;
                invM.Scale(1.0f / bounds.Width, 1.0f / bounds.Height);
                invM.Translate(-bounds.X, -bounds.Y);

                brush.MultiplyTransform(&invM, Gdiplus::MatrixOrderAppend);
                brush.MultiplyTransform(&m, Gdiplus::MatrixOrderAppend);
            }

            // 5. Vẽ hình
            // Lưu ý: Không nhân thêm 'transform' của shape ở đây nếu 'g' đã được áp dụng transform chung
            g.FillPath(&brush, &path);
        }
        else if (useRadialGradient && !radColors.empty()) {
            Gdiplus::RectF bounds;
            path.GetBounds(&bounds, nullptr, nullptr);

            Gdiplus::GraphicsPath gradPath;
            gradPath.AddEllipse(-1.0f, -1.0f, 2.0f, 2.0f);
            Matrix mat;
            if (radUserSpace) {
                // userSpaceOnUse: Đơn vị Pixel
                mat.Translate(radCx, radCy);
                mat.Scale(radR, radR);
                gradPath.AddEllipse(radCx - radR, radCy - radR, radR * 2, radR * 2);
                if (hasGradTransform) gradPath.Transform(&gradMatrix);
            }
            else {
                // objectBoundingBox: Đơn vị 0..1
                gradPath.AddEllipse(radCx - radR, radCy - radR, radR * 2, radR * 2);

                if (hasGradTransform) mat.Multiply(&gradMatrix, Gdiplus::MatrixOrderAppend);

                mat.Translate(bounds.X + radCx * bounds.Width, bounds.Y + radCy * bounds.Height);
                mat.Scale(radR* bounds.Width, radR* bounds.Height);
                gradPath.Transform(&mat);
            }

            Gdiplus::PathGradientBrush pthGrBrush(&gradPath);

            // Tính tâm (Focal Point)
            Gdiplus::PointF centerPoint(radFx, radFy);
            if (radUserSpace) {
                if (hasGradTransform) gradMatrix.TransformPoints(&centerPoint);
            }
            else {
                Gdiplus::Matrix pointMat;
                if (hasGradTransform) pointMat.Multiply(&gradMatrix, Gdiplus::MatrixOrderAppend);
                pointMat.Scale(bounds.Width, bounds.Height, Gdiplus::MatrixOrderAppend);
                pointMat.Translate(bounds.X, bounds.Y, Gdiplus::MatrixOrderAppend);
                pointMat.TransformPoints(&centerPoint);
            }
            pthGrBrush.SetCenterPoint(centerPoint);

            // Xử lý màu (Đảo ngược theo logic đã fix)
            struct StopEntry { float offset; Gdiplus::Color col; };
            std::vector<StopEntry> finalStops;
            for (size_t k = 0; k < radColors.size(); ++k) {
                float rawOff = (k < radOffsets.size()) ? radOffsets[k] : 0.0f;
                int a = (radColors[k].GetAlpha() * finalFillAlpha) / 255;
                // Offset đã đảo ở svgdocument, giữ nguyên
                float gdiOffset = rawOff;
                if (gdiOffset < 0.0f) gdiOffset = 0.0f; else if (gdiOffset > 1.0f) gdiOffset = 1.0f;
                finalStops.push_back({ gdiOffset, Gdiplus::Color((BYTE)a, radColors[k].GetR(), radColors[k].GetG(), radColors[k].GetB()) });
            }
            // Sort
            std::sort(finalStops.begin(), finalStops.end(), [](const StopEntry& a, const StopEntry& b) { return a.offset < b.offset; });

            std::vector<Gdiplus::Color> cols; std::vector<float> offs;
            if (!finalStops.empty() && finalStops.front().offset > 0.001f) { cols.push_back(finalStops.front().col); offs.push_back(0.0f); }
            for (const auto& e : finalStops) { cols.push_back(e.col); offs.push_back(e.offset); }
            if (!finalStops.empty() && finalStops.back().offset < 0.999f) { cols.push_back(finalStops.back().col); offs.push_back(1.0f); }

            if (!cols.empty()) {
                if (cols.size() == 1) { cols.push_back(cols[0]); offs.push_back(1.0f); }
                pthGrBrush.SetInterpolationColors(cols.data(), offs.data(), (INT)cols.size());

                // Tô nền pad
                if (cols.size() > 0) {
                    Gdiplus::SolidBrush bgBrush(cols[0]);
                    g.FillPath(&bgBrush, &path);
                }
                g.FillPath(&pthGrBrush, &path);
            }
        }
        else {
            // Solid Fill (không dùng gradient)
            Gdiplus::SolidBrush fillBrush(Gdiplus::Color(finalFillAlpha,
                fillColor.GetR(), fillColor.GetG(), fillColor.GetB()));
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