#ifndef SVGSHAPE_H
#define SVGSHAPE_H
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <iostream>
#include <string>
#pragma comment (lib, "gdiplus.lib")
using namespace Gdiplus;
class SVGSHAPE {
protected:
    // ===== Visibility / display =====
    bool visible;        // true = render, false = skip
    bool displayNone;    // true = giống display="none"

    // ===== Fill style =====
    bool hasFill;                // fill != "none"
    Gdiplus::Color fillColor;    // màu fill (ARGB sau khi tính alpha)
    float fillOpacity;           // 0..1 (riêng fill)

    // ===== Stroke style =====
    bool hasStroke;              // stroke != "none"
    Gdiplus::Color strokeColor;  // màu stroke (ARGB sau alpha)
    float strokeOpacity;         // 0..1
    float strokeWidth;           // stroke-width

    // ===== Overall opacity =====
    float overallOpacity;        // opacity của cả shape 0..1

    // ===== Transform (chưa parse nâng cao) =====
    // Với SVG thật, ta có thể có ma trận 2D. Ở đây mình chỉ để chỗ.
    // Sau này ta có thể set có/không transform.
    bool hasTransform;
    Gdiplus::Matrix transform;

public:
    SVGSHAPE();
    virtual ~SVGSHAPE() {}
    // ------------- visibility / display -------------
    void SetVisible(bool v);
    void SetDisplayNone(bool d);
    void EnableFill(BYTE r, BYTE g, BYTE b);
    void DisableFill();
    void SetFillOpacity(float op);
    // ------------- stroke -------------
    void EnableStroke(BYTE r, BYTE g, BYTE b, float width);
    void DisableStroke();
    void SetStrokeOpacity(float op);
    void SetStrokeWidth(float w);
    // ------------- overall opacity -------------
    void SetOverallOpacity(float op);
    // ------------- Render wrapper -------------
    // Quan trọng:
    //  - Tính alpha cuối cho fill/stroke dựa trên fill-opacity,
    //    stroke-opacity và overall opacity
    //  - Apply transform nếu có
    void Draw(Gdiplus::Graphics& g) const;

    // Hàm thực sự vẽ shape -> lớp con phải override
    // finalFillAlpha / finalStrokeAlpha là alpha đã trộn opacity

    virtual void DrawImpl(Gdiplus::Graphics& g, BYTE finalFillAlpha, BYTE finalStrokeAlpha) const = 0;
};
#endif // !SVGSHAPE_H
