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

void SVGTEXT::DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const {
    if (text.empty()) return;

    // --- 1) Chuẩn bị chuỗi & font (GIỮ NGUYÊN) ---
    std::wstring wText = Utf8ToWstring(text);
    std::wstring wFont = Utf8ToWstring(fontFamily);

    FontFamily fam(wFont.c_str());
    const FontFamily* ff = (fam.GetLastStatus() == Ok) ? &fam : FontFamily::GenericSerif();

    INT style = FontStyleRegular;
    if (fontStyle == "italic") style |= FontStyleItalic;
    if (fontWeight == "bold")  style |= FontStyleBold;

    INT em = ff->GetEmHeight(style);
    INT asc = ff->GetCellAscent(style);
    float ascentPx = (em > 0) ? (fontSize * float(asc) / float(em)) : 0.0f;

    // --- 2) Tạo Path tại gốc (0,0) (GIỮ NGUYÊN) ---
    GraphicsPath path;
    StringFormat fmt(StringFormatFlagsNoClip | StringFormatFlagsNoWrap | StringFormatFlagsMeasureTrailingSpaces, LANG_NEUTRAL);

    path.AddString(
        wText.c_str(), (INT)wText.size(),
        ff, style, fontSize,
        PointF(0.0f, 0.0f), &fmt
    );

    // --- 3) Layout: Tính toán vị trí ---
    RectF b;
    path.GetBounds(&b, nullptr, nullptr);

    // [QUAN TRỌNG] Đổi tên biến 'dx' cũ thành 'anchorOffset' để không trùng với this->dx
    float anchorOffset = 0.0f;
    if (textAnchor == "middle") anchorOffset = b.Width * 0.5f;
    else if (textAnchor == "end") anchorOffset = b.Width;

    auto snap = [](float v) { return std::floor(v) + 0.5f; };

    // [CÔNG THỨC MỚI]
    // Vị trí thực tế = (x + dx) - anchorOffset
    // Vị trí dòng kẻ = (y + dy) - ascentPx
    float finalX = x + dx;
    float finalY = y + dy;

    Matrix layoutMtx;
    layoutMtx.Translate(
        snap(finalX - anchorOffset),
        snap(finalY - ascentPx)
    );
    path.Transform(&layoutMtx);

    // --- Phần còn lại (Render) GIỮ NGUYÊN ---
    SmoothingMode oldSmooth = g.GetSmoothingMode();
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    if (hasFill && finalFillAlpha > 0) {
        Color c(finalFillAlpha, fillColor.GetR(), fillColor.GetG(), fillColor.GetB());
        SolidBrush br(c);
        g.FillPath(&br, &path);
    }

    if (hasStroke && finalStrokeAlpha > 0 && strokeWidth > 0) {
        Color c(finalStrokeAlpha, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB());
        Pen pen(c, strokeWidth);
        pen.SetAlignment(PenAlignmentCenter);
        pen.SetMiterLimit(strokeMiterLimit);

        // ... (Code xử lý linejoin cũ giữ nguyên) ...
        std::string joinLower = strokeLinejoin;
        for (auto& ch : joinLower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (joinLower == "round") pen.SetLineJoin(LineJoinRound);
        else if (joinLower == "bevel") pen.SetLineJoin(LineJoinBevel);
        else pen.SetLineJoin(LineJoinMiter);

        g.DrawPath(&pen, &path);
    }

    g.SetSmoothingMode(oldSmooth);
}