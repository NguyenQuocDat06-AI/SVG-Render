#ifndef TEXT_H
#define TEXT_H
#include "svgshape.h"
class SVGTEXT : public SVGSHAPE {
protected:
    float x;
    float y;
    float dx;
    float dy;
    std::string text;
    std::string fontFamily;
    float fontSize;
    std::string fontWeight; // "normal", "bold"
    std::string fontStyle;  // "normal", "italic"
    std::string textAnchor; // "start", "middle", "end"

public:
    SVGTEXT(float x, float y, float dx, float dy, const std::string& content);

    void SetText(const std::string& content);
    void SetPosition(float x, float y);
    void SetFont(const std::string& family, float size);
    void SetStyle(const std::string& weight, const std::string& style);
    void SetAnchor(const std::string& anchor);

protected:
    void DrawImpl(Gdiplus::Graphics& g, BYTE fillA, BYTE strokeA) const override;
};
#endif