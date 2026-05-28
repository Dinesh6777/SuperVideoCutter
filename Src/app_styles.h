#pragma once
#include <windows.h>
#include <gdiplus.h>

// Control IDs
#define IDC_PREVIEW_WINDOW    101
#define IDC_BTN_PLAY_PAUSE    102
#define IDC_BTN_BACK_5        103
#define IDC_BTN_BACK_3        104
#define IDC_BTN_FORWARD_3     105
#define IDC_BTN_FORWARD_5     106
#define IDC_BTN_BROWSE        107
#define IDC_BTN_SET_START     108
#define IDC_BTN_SET_END       109
#define IDC_BTN_ADD_CUT       110
#define IDC_BTN_CUT_ALL       111
#define IDC_TXT_START         112
#define IDC_TXT_END           113
#define IDC_CHK_KEYFRAMES     114
#define IDC_SEEKBAR           115
#define IDC_CUTS_LIST         116

// Colors - Sleek Premium Light Mode
#define COLOR_BG            RGB(243, 244, 246)    // Clean, soft off-white/light gray background
#define COLOR_CARD          RGB(255, 255, 255)    // Crisp white card background
#define COLOR_ACCENT        RGB(58, 134, 255)     // Vibrant Blue accent
#define COLOR_ACCENT_HOVER  RGB(30, 100, 240)     // Deep Blue accent for hover
#define COLOR_TEXT_WHITE    RGB(255, 255, 255)    // Clean white text
#define COLOR_TEXT_DARK     RGB(27, 30, 33)       // Premium dark charcoal text
#define COLOR_TEXT_MUTED    RGB(100, 110, 120)    // Muted gray text
#define COLOR_BORDER        RGB(209, 213, 219)    // Subtle light gray border
#define COLOR_CUT_HIGHLIGHT RGB(239, 71, 111)     // Soft crimson/coral highlight
#define COLOR_GREEN         RGB(40, 167, 69)      // Success green
#define COLOR_PROCESS       RGB(58, 134, 255)     // Vibrant Blue for primary action button
#define COLOR_PROCESS_HOVER RGB(30, 100, 240)

// Fonts & Pens/Brushes Context
struct AppTheme {
    HFONT hFontNormal;
    HFONT hFontSmall;
    HFONT hFontBold;
    HFONT hFontTitle;

    HBRUSH hBrushBg;
    HBRUSH hBrushCard;
    HBRUSH hBrushAccent;
    HBRUSH hBrushAccentHover;
    HBRUSH hBrushBorder;
    HBRUSH hBrushTextWhite;
    HBRUSH hBrushTextDark;
    HBRUSH hBrushTextMuted;
    HBRUSH hBrushHighlight;
    HBRUSH hBrushGreen;
    HBRUSH hBrushProcess;
    HBRUSH hBrushProcessHover;

    HPEN hPenBorder;
    HPEN hPenAccent;
    HPEN hPenHighlight;

    void Initialize() {
        hFontNormal = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        hFontSmall = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        hFontBold = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        hFontTitle = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        hBrushBg = CreateSolidBrush(COLOR_BG);
        hBrushCard = CreateSolidBrush(COLOR_CARD);
        hBrushAccent = CreateSolidBrush(COLOR_ACCENT);
        hBrushAccentHover = CreateSolidBrush(COLOR_ACCENT_HOVER);
        hBrushBorder = CreateSolidBrush(COLOR_BORDER);
        hBrushTextWhite = CreateSolidBrush(COLOR_TEXT_WHITE);
        hBrushTextDark = CreateSolidBrush(COLOR_TEXT_DARK);
        hBrushTextMuted = CreateSolidBrush(COLOR_TEXT_MUTED);
        hBrushHighlight = CreateSolidBrush(COLOR_CUT_HIGHLIGHT);
        hBrushGreen = CreateSolidBrush(COLOR_GREEN);
        hBrushProcess = CreateSolidBrush(COLOR_PROCESS);
        hBrushProcessHover = CreateSolidBrush(COLOR_PROCESS_HOVER);

        hPenBorder = CreatePen(PS_SOLID, 1, COLOR_BORDER);
        hPenAccent = CreatePen(PS_SOLID, 2, COLOR_ACCENT);
        hPenHighlight = CreatePen(PS_SOLID, 2, COLOR_CUT_HIGHLIGHT);
    }

    void Cleanup() {
        DeleteObject(hFontNormal);
        DeleteObject(hFontSmall);
        DeleteObject(hFontBold);
        DeleteObject(hFontTitle);

        DeleteObject(hBrushBg);
        DeleteObject(hBrushCard);
        DeleteObject(hBrushAccent);
        DeleteObject(hBrushAccentHover);
        DeleteObject(hBrushBorder);
        DeleteObject(hBrushTextWhite);
        DeleteObject(hBrushTextDark);
        DeleteObject(hBrushTextMuted);
        DeleteObject(hBrushHighlight);
        DeleteObject(hBrushGreen);
        DeleteObject(hBrushProcess);
        DeleteObject(hBrushProcessHover);

        DeleteObject(hPenBorder);
        DeleteObject(hPenAccent);
        DeleteObject(hPenHighlight);
    }
};

extern AppTheme g_Theme;
