#include "text.h"
static std::wstring Utf8ToWstring(const std::string& str) {
    if (str.empty()) return std::wstring();

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], wideLen);

    if (!wstr.empty() && wstr.back() == L'\0')
        wstr.pop_back();

    return wstr;
}

SVGTEXT::SVGTEXT(float x, float y, const std::string& content)
    : x(x), y(y), text(content),
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

void SVGTEXT::DrawImpl(Gdiplus::Graphics& g, BYTE fillA, BYTE strokeA) const {
    using namespace Gdiplus;
    if (text.empty()) return;

    // --- 1) Chuẩn bị chuỗi & font ---
    std::wstring wText = Utf8ToWstring(text);
    std::wstring wFont = Utf8ToWstring(fontFamily);

    FontFamily fam(wFont.c_str());
    const FontFamily* ff = (fam.GetLastStatus() == Ok) ? &fam : FontFamily::GenericSerif();

    INT style = FontStyleRegular;
    if (fontStyle == "italic") style |= FontStyleItalic;
    if (fontWeight == "bold")  style |= FontStyleBold;

    // Baseline (SVG dùng baseline): y là baseline -> cần ascent(px)
    INT em = ff->GetEmHeight(style);
    INT asc = ff->GetCellAscent(style);
    float ascentPx = (em > 0) ? (fontSize * float(asc) / float(em)) : 0.0f;

    // --- 2) Dựng path tại gốc (0,0) để đo chính xác ---
    GraphicsPath path;
    StringFormat fmt(
        StringFormatFlagsNoClip
        | StringFormatFlagsNoWrap
        | StringFormatFlagsMeasureTrailingSpaces,
        LANG_NEUTRAL
    );
    path.AddString(
        wText.c_str(),
        (INT)wText.size(),
        ff,
        style,
        fontSize,                 // em-size (pixel)
        PointF(0.0f, 0.0f),
        &fmt
    );

    // --- 3) Tính dịch theo text-anchor & baseline ---
    RectF b; Matrix id;
    path.GetBounds(&b, &id, nullptr);

    float dx = 0.0f;
    if (textAnchor == "middle") dx = b.Width * 0.5f;
    else if (textAnchor == "end") dx = b.Width;

    // Snap về nửa-pixel để AA của fill/stroke trùng nhau
    auto snap = [](float v) { return std::floor(v) + 0.5f; };

    Matrix mtx;
    mtx.Translate(
        snap(x - (b.X + dx)),
        snap((y - ascentPx) - b.Y)
    );
    path.Transform(&mtx);

    // --- 4) Đồng bộ cấu hình raster cho cả fill & stroke ---
    SmoothingMode oldSmooth = g.GetSmoothingMode();
    PixelOffsetMode oldPix = g.GetPixelOffsetMode();
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);

    // --- 5) Vẽ ---
    if (hasFill && fillA > 0) {
        Color c(fillA, fillColor.GetR(), fillColor.GetG(), fillColor.GetB());
        SolidBrush br(c);
        g.FillPath(&br, &path);
    }
    if (hasStroke && strokeA > 0 && strokeWidth > 0) {
        Color c(strokeA, strokeColor.GetR(), strokeColor.GetG(), strokeColor.GetB());
        Pen pen(c, strokeWidth);
        pen.SetAlignment(PenAlignmentCenter); // mặc định, giữ stroke cân
        pen.SetLineJoin(LineJoinRound);       // hạn chế răng cưa ở góc
        pen.SetMiterLimit(4.0f);
        g.DrawPath(&pen, &path);
    }

    // --- 6) Khôi phục ---
    g.SetPixelOffsetMode(oldPix);
    g.SetSmoothingMode(oldSmooth);
}