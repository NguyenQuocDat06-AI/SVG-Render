#include "text.h"
#include <algorithm>
#include <cctype>
static std::wstring Utf8ToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], wideLen);

    if (!wstr.empty() && wstr.back() == L'\0')
        wstr.pop_back();

    return wstr;
}

SVGTEXT::SVGTEXT(float x, float y,float dx,float dy,const std::string& content)
    : x(x), y(y), dx(dx), dy(dy), text(content),
    fontFamily("Arial"), fontSize(16.0f),
    fontWeight("normal"), fontStyle("normal"),
    textAnchor("start") {
}

void SVGTEXT::SetText(const std::string& content) {
    text = content;
}

void SVGTEXT::SetPosition(float x, float y) {
    this->x = x;
    this->y = y;

}

void SVGTEXT::SetFont(const std::string& family, float size) {
    fontFamily = family;
    fontSize = size;
}

void SVGTEXT::SetStyle(const std::string& weight, const std::string& style) {
    fontWeight = weight;
    fontStyle = style;
}

void SVGTEXT::SetAnchor(const std::string& anchor) {
    textAnchor = anchor;
}

float Snap(float v) {
    return std::floor(v) + 0.5f;
}

void SVGTEXT::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
    if (text.empty()) return;

    // --- 1. CHUẨN BỊ FONT & STYLE ---
    std::wstring wText = Utf8ToWstring(text);
    std::wstring wFont = Utf8ToWstring(fontFamily);

    // Tạo FontFamily, fallback về GenericSansSerif nếu không tìm thấy font
    FontFamily fam(wFont.c_str());
    const FontFamily* ff = (fam.GetLastStatus() == Ok) ? &fam : FontFamily::GenericSansSerif();

    INT style = FontStyleRegular;
    if (fontWeight == "bold") style |= FontStyleBold;
    if (fontStyle == "italic") style |= FontStyleItalic;

    // --- 2. TÍNH TOÁN BASELINE (QUAN TRỌNG ĐỂ KHÔNG BỊ LỆCH DÒNG) ---
    // GDI+ vẽ từ Top-Left, SVG vẽ từ Baseline.
    // Cần tính khoảng cách từ đỉnh chữ xuống baseline (Ascent)
    UINT16 emHeight = ff->GetEmHeight(style);
    UINT16 cellAscent = ff->GetCellAscent(style);

    // Đổi đơn vị từ Design Unit sang Pixel
    float ascentPx = 0.0f;
    if (emHeight > 0) {
        ascentPx = fontSize * (float)cellAscent / (float)emHeight;
    }

    // --- 3. TẠO PATH VỚI STRINGFORMAT CHUẨN (FIX LỖI PADDING) ---
    GraphicsPath path;

    // Sử dụng GenericTypographic: Cái này quan trọng nhất để fix lỗi "chữ bị lệch"
    // Nó loại bỏ các khoảng đệm thừa mặc định của GDI+
    const StringFormat* genericFmt = StringFormat::GenericTypographic();
    StringFormat fmt(genericFmt);

    // Flag: MeasureTrailingSpaces để giữ khoảng trắng cuối câu nếu có
    fmt.SetFormatFlags(StringFormatFlagsMeasureTrailingSpaces | StringFormatFlagsNoClip);

    // Vẽ chữ tại gốc tọa độ (0,0) trước, sau đó mới dời đi
    path.AddString(
        wText.c_str(),
        (INT)wText.size(),
        ff,
        style,
        fontSize,           // Size tính bằng Pixel
        PointF(0.0f, 0.0f), // Vẽ tạm tại 0,0
        &fmt
    );

    // --- 4. TÍNH TOÁN VỊ TRÍ (LAYOUT) ---
    RectF b;
    // Lấy khung bao chính xác (Tight bounding box)
    path.GetBounds(&b, nullptr, nullptr);

    // Xử lý text-anchor (start, middle, end)
    float anchorOffset = 0.0f;
    if (textAnchor == "middle") {
        anchorOffset = b.Width / 2.0f;
    }
    else if (textAnchor == "end") {
        anchorOffset = b.Width;
    }

    // Tính tọa độ đích
    // SVG x, y: Là điểm bắt đầu của baseline
    // dx, dy: Là khoảng dịch chuyển tương đối (thường dùng trong tspan)
    float finalX = x + dx;
    float finalY = y + dy;

    // --- 5. BIẾN ĐỔI MA TRẬN (TRANSFORM) ---
    Matrix layoutMtx;

    // Dịch chuyển đến vị trí mong muốn:
    // X = Vị trí x - (độ lệch do căn lề)
    // Y = Vị trí y - (chiều cao phần trên chữ - ascent) -> Để đưa baseline về đúng y
    layoutMtx.Translate(
        Snap(finalX - anchorOffset),
        Snap(finalY - ascentPx)
    );

    // Áp dụng biến đổi cho đường dẫn chữ
    path.Transform(&layoutMtx);

    // --- 6. VẼ (RENDER) ---
    // Lưu chế độ cũ
    SmoothingMode oldSmooth = g.GetSmoothingMode();
    g.SetSmoothingMode(SmoothingModeAntiAlias); // Bật khử răng cưa cho chữ đẹp

    // 6a. TÔ MÀU (FILL)
    // Chỉ tô nếu hasFill = true VÀ Alpha > 0
    if (hasFill && finalFillAlpha > 0) {
        Color c(finalFillAlpha, fillColor.GetR(), fillColor.GetG(), fillColor.GetB());
        SolidBrush brush(c);
        g.FillPath(&brush, &path);
    }

    // 6b. VẼ VIỀN (STROKE)
    // Chỉ vẽ nếu hasStroke = true VÀ Alpha > 0 VÀ độ dày > 0
    if (hasStroke && finalStrokeAlpha > 0 && strokeWidth > 0.0f) {
        Color c(finalStrokeAlpha, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB());
        Pen pen(c, strokeWidth);

        // Cấu hình nét vẽ
        pen.SetMiterLimit(strokeMiterLimit);

        // Xử lý LineJoin (Góc nối)
        // Chuyển về chữ thường để so sánh
        std::string joinType = strokeLinejoin;
        std::transform(joinType.begin(), joinType.end(), joinType.begin(), ::tolower);

        if (joinType == "round") pen.SetLineJoin(LineJoinRound);
        else if (joinType == "bevel") pen.SetLineJoin(LineJoinBevel);
        else pen.SetLineJoin(LineJoinMiter);

        g.DrawPath(&pen, &path);
    }

    // Khôi phục chế độ cũ
    g.SetSmoothingMode(oldSmooth);
}