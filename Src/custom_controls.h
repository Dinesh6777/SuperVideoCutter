#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

// Forward declarations
struct VideoClip {
    std::string startTime;
    std::string endTime;
};

// Main style registration function
void RegisterCustomControls(HINSTANCE hInstance);

// Button subclass helper to make any button beautiful and dark-themed
void SubclassButton(HWND hWndButton, COLORREF normalColor, COLORREF hoverColor, COLORREF textColor);

// Custom control class names
#define WC_CUSTOM_SEEKBAR    L"CustomSeekbar"
#define WC_CUSTOM_CUTS_LIST  L"CustomCutsList"

// Notifications sent by CustomSeekbar to parent via WM_COMMAND
#define SBN_SEEKING           1001 // Sent while dragging
#define SBN_SEEK_DONE         1002 // Sent when drag stops

// Notifications sent by CustomCutsList to parent
#define CLN_PREVIEW_CLIP      2001
#define CLN_DELETE_CLIP       2002

// Messages for CustomSeekbar
#define SBM_SETPOSITION       (WM_USER + 101) // wParam = 1 (only if not dragging), 0 (always), lParam = position in ms
#define SBM_SETDURATION       (WM_USER + 102) // lParam = duration in ms
#define SBM_GETPOSITION       (WM_USER + 103) // returns position in ms

