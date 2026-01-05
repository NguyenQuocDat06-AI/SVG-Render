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
        // 1. Lấy khung bao và mở rộng nhẹ để khử răng cưa
        Gdiplus::RectF bounds;
        path.GetBounds(&bounds);
        bounds.Inflate(1.0f, 1.0f);

        // Tránh lỗi nếu Path quá nhỏ
        if (bounds.Width <= 0.0f) bounds.Width = 1.0f;
        if (bounds.Height <= 0.0f) bounds.Height = 1.0f;

        if (useLinearGradient && !gColors.empty()) {
            // --- XỬ LÝ LINEAR GRADIENT ---
            // (Giữ nguyên logic Linear của bạn nếu nó đang ổn, 
            // hoặc cập nhật tương tự logic Bounds mới ở đây)

            Gdiplus::PointF p1(
                bounds.X + gStart.X * bounds.Width,
                bounds.Y + gStart.Y * bounds.Height
            );
            Gdiplus::PointF p2(
                bounds.X + gEnd.X * bounds.Width,
                bounds.Y + gEnd.Y * bounds.Height
            );

            std::vector<Gdiplus::Color> finalCols;
            for (const auto& c : gColors) {
                BYTE combinedAlpha = (BYTE)((c.GetAlpha() * finalFillAlpha) / 255);
                finalCols.emplace_back(combinedAlpha, c.GetR(), c.GetG(), c.GetB());
            }

            Gdiplus::LinearGradientBrush brush(p1, p2, finalCols.front(), finalCols.back());

            if (finalCols.size() == gOffsets.size()) {
                std::vector<float> finalOffs = gOffsets;
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

            if (hasGradTransform) {
                Gdiplus::Matrix m;
                m.Translate(bounds.X, bounds.Y);
                m.Scale(bounds.Width, bounds.Height);
                m.Multiply(&gradMatrix, Gdiplus::MatrixOrderPrepend);

                Gdiplus::Matrix invM;
                invM.Scale(1.0f / bounds.Width, 1.0f / bounds.Height);
                invM.Translate(-bounds.X, -bounds.Y);

                brush.MultiplyTransform(&invM, Gdiplus::MatrixOrderAppend);
                brush.MultiplyTransform(&m, Gdiplus::MatrixOrderAppend);
            }

            brush.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
            g.FillPath(&brush, &path);
        }
        else if (useRadialGradient && !radColors.empty()) {
            // --- XỬ LÝ RADIAL GRADIENT (LOGIC CODE 1 CHUẨN) ---

            // 1. Chuẩn bị các biến kích thước thực tế (Pixel)
            float rCx = radCx;
            float rCy = radCy;
            float rR = radR;
            float rFx = radFx;
            float rFy = radFy;

            // Tính đường chéo của Bounds
            float bboxDiag = sqrt(bounds.Width * bounds.Width + bounds.Height * bounds.Height);

            // Xử lý objectBoundingBox (Theo logic Code 1)
            if (!radUserSpace) {
                rCx = bounds.X + rCx * bounds.Width;
                rCy = bounds.Y + rCy * bounds.Height;

                // Scale bán kính theo đường chéo hộp đơn vị (Code 1)
                float dim = bboxDiag / sqrt(2.0f);
                rR = rR * dim;

                if (rFx == FLT_MAX) rFx = rCx;
                else rFx = bounds.X + rFx * bounds.Width;

                if (rFy == FLT_MAX) rFy = rCy;
                else rFy = bounds.Y + rFy * bounds.Height;
            }
            else {
                if (rFx == FLT_MAX) rFx = rCx;
                if (rFy == FLT_MAX) rFy = rCy;
            }

            if (rR <= 0.1f) rR = 0.1f;

            // 2. TÍNH TOÁN BÁN KÍNH VẼ (DrawRadius) & TỈ LỆ (ScaleRatio)
            // Vẽ hình tròn lớn gấp 1.5 lần đường chéo để xử lý Pad
            float drawRadius = (std::max)(rR, bboxDiag) * 1.5f;
            float scaleRatio = rR / drawRadius;

            // 3. Tạo GraphicsPath với DrawRadius
            Gdiplus::GraphicsPath gradPath;
            // Tạo shape tại 0,0 trước để dễ Transform nếu cần, sau đó mới dời về vị trí thật
            // Hoặc tạo thẳng tại vị trí nếu xử lý Matrix riêng.
            // Ở đây ta tạo thẳng tại vị trí rCx, rCy nhưng với bán kính LỚN
            gradPath.AddEllipse(rCx - drawRadius, rCy - drawRadius, drawRadius * 2, drawRadius * 2);

            // 4. Xử lý Matrix (Gradient Transform) - FIX LỖI COMPILER
            if (hasGradTransform) {
                if (!radUserSpace) {
                    // Logic: Reset path về gốc -> Transform -> Move về vị trí thật

                    // FIX LỖI 1: Không copy Matrix trực tiếp. Tạo mới và Multiply.
                    Gdiplus::Matrix svgM;
                    svgM.Multiply(&gradMatrix);

                    gradPath.Reset();
                    gradPath.AddEllipse(-drawRadius, -drawRadius, drawRadius * 2, drawRadius * 2);
                    gradPath.Transform(&svgM);

                    // FIX LỖI 2: GraphicsPath không có Translate. Dùng Matrix.
                    Gdiplus::Matrix moveMat;
                    moveMat.Translate(rCx, rCy);
                    gradPath.Transform(&moveMat);
                }
                else {
                    gradPath.Transform(&gradMatrix);
                }
            }

            Gdiplus::PathGradientBrush pthGrBrush(&gradPath);

            // 5. Thiết lập Focal Point
            Gdiplus::PointF centerPoint(rFx, rFy);
            if (hasGradTransform) {
                if (!radUserSpace) {
                    Gdiplus::Matrix svgM;
                    svgM.Multiply(&gradMatrix);
                    // Áp dụng offset (Translate) vào Matrix để transform điểm Focal
                    svgM.Translate(rCx, rCy, Gdiplus::MatrixOrderAppend);
                    // svgM.TransformPoints(&centerPoint); // (Bỏ comment nếu cần transform cả Focal Point)
                }
                else {
                    gradMatrix.TransformPoints(&centerPoint);
                }
            }
            pthGrBrush.SetCenterPoint(centerPoint);

            // --- 6. XỬ LÝ MÀU & OFFSET (THEO CODE 1) ---

            // B1: Sort Stops
            struct StopEntry { float offset; Gdiplus::Color col; };
            std::vector<StopEntry> sortedStops;
            for (size_t k = 0; k < radColors.size(); ++k) {
                float off = (k < radOffsets.size()) ? radOffsets[k] : 0.0f;
                // Clamp 0..1
                if (off < 0.0f) off = 0.0f; if (off > 1.0f) off = 1.0f;

                int a = (radColors[k].GetAlpha() * finalFillAlpha) / 255;
                sortedStops.push_back({ off, Gdiplus::Color((BYTE)a, radColors[k].GetR(), radColors[k].GetG(), radColors[k].GetB()) });
            }
            std::sort(sortedStops.begin(), sortedStops.end(), [](const StopEntry& a, const StopEntry& b) { return a.offset < b.offset; });

            // B2: Map Offset theo ScaleRatio (0 -> 1)
            std::vector<Gdiplus::Color> tempColors;
            std::vector<float> tempPos;

            if (!sortedStops.empty() && sortedStops.front().offset > 0.0001f) {
                tempPos.push_back(0.0f);
                tempColors.push_back(sortedStops.front().col);
            }

            for (const auto& s : sortedStops) {
                tempPos.push_back(s.offset * scaleRatio);
                tempColors.push_back(s.col);
            }

            // B3: Handle Pad (Kéo dài màu cuối ra 1.0 trên hình tròn to)
            if (scaleRatio < 0.999f) {
                Gdiplus::Color lastColor = tempColors.back();
                if (tempPos.back() < scaleRatio) {
                    tempPos.push_back(scaleRatio);
                    tempColors.push_back(lastColor);
                }
                tempPos.push_back(1.0f);
                tempColors.push_back(lastColor);
            }
            else {
                if (tempPos.back() < 1.0f) {
                    tempPos.push_back(1.0f);
                    tempColors.push_back(tempColors.back());
                }
            }

            // B4: ĐẢO NGƯỢC MẢNG (Reversing) 
            // SVG (Tâm->Biên) sang GDI+ (Biên->Tâm)
            int count = (int)tempColors.size();
            std::vector<Gdiplus::Color> finalColors;
            std::vector<float> finalPos;

            for (int i = count - 1; i >= 0; --i) {
                float newPos = 1.0f - tempPos[i];
                Gdiplus::Color newCol = tempColors[i];

                if (!finalPos.empty() && newPos <= finalPos.back()) {
                    newPos = finalPos.back() + 0.0001f;
                }
                // Clamp float error
                if (newPos < 0.0f) newPos = 0.0f;
                if (newPos > 1.0f) newPos = 1.0f;

                if (finalPos.empty()) newPos = 0.0f;

                finalPos.push_back(newPos);
                finalColors.push_back(newCol);
            }

            // Đảm bảo phần tử cuối là 1.0
            if (finalPos.back() < 1.0f) finalPos.back() = 1.0f;
            if (finalPos.size() < 2) {
                finalPos.push_back(1.0f);
                finalColors.push_back(finalColors.back());
            }

            // 7. Thiết lập Brush
            pthGrBrush.SetCenterColor(finalColors.back());
            pthGrBrush.SetInterpolationColors(finalColors.data(), finalPos.data(), (INT)finalColors.size());

            // WrapMode Clamp (từ Code 1)
            pthGrBrush.SetWrapMode(Gdiplus::WrapModeClamp);

            // Đã xóa SetGammaCorrection theo yêu cầu

            g.FillPath(&pthGrBrush, &path);
        }
        else {
            // Solid Fill
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