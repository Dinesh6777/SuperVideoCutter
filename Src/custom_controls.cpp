#include "custom_controls.h"
#include "app_styles.h"
#include <commctrl.h>
#include <windowsx.h>
#include <vector>
#include <string>

// Global theme instance
AppTheme g_Theme;

// Button Subclass Data Structure
struct ButtonSubclassInfo {
    COLORREF normalColor;
    COLORREF hoverColor;
    COLORREF textColor;
    bool isHovered;
    bool isPressed;
};

// Button Subclass Proc
LRESULT CALLBACK ButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    ButtonSubclassInfo* info = reinterpret_cast<ButtonSubclassInfo*>(dwRefData);

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            if (!info->isHovered) {
                info->isHovered = true;
                TRACKMOUSEEVENT tme = { sizeof(tme) };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            info->isHovered = false;
            info->isPressed = false;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN: {
            info->isPressed = true;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (info->isPressed) {
                info->isPressed = false;
                InvalidateRect(hWnd, NULL, FALSE);
                // Trigger button click via standard WM_COMMAND in parent
                HWND hParent = GetParent(hWnd);
                int id = GetDlgCtrlID(hWnd);
                PostMessage(hParent, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)hWnd);
            }
            return 0;
        }
        case WM_SETFOCUS:
        case WM_KILLFOCUS: {
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        case WM_ERASEBKGND:
            return 1; // Handled
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            // Double Buffering
            HDC hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HGDIOBJ hOldBmp = SelectObject(hMemDC, hMemBmp);

            // Draw Background (parent's card color or background color)
            HBRUSH hBgBrush = CreateSolidBrush(COLOR_BG);
            FillRect(hMemDC, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            // Select color based on state
            COLORREF btnColor = info->normalColor;
            if (info->isPressed) {
                // Darken slightly
                btnColor = RGB(GetRValue(info->normalColor) * 0.8, GetGValue(info->normalColor) * 0.8, GetBValue(info->normalColor) * 0.8);
            } else if (info->isHovered) {
                btnColor = info->hoverColor;
            }

            // Draw Rounded Rectangle Button
            HBRUSH hBtnBrush = CreateSolidBrush(btnColor);
            HGDIOBJ hOldBrush = SelectObject(hMemDC, hBtnBrush);
            
            // Draw subtle border
            HPEN hBorderPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
            HGDIOBJ hOldPen = SelectObject(hMemDC, hBorderPen);

            RoundRect(hMemDC, rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 2, 8, 8);

            // Draw Button Text
            wchar_t text[256];
            GetWindowTextW(hWnd, text, 256);

            SetBkMode(hMemDC, TRANSPARENT);
            SetTextColor(hMemDC, info->textColor);
            SelectObject(hMemDC, g_Theme.hFontNormal);

            DrawTextW(hMemDC, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Clean up GDI Objects
            SelectObject(hMemDC, hOldBrush);
            SelectObject(hMemDC, hOldPen);
            DeleteObject(hBtnBrush);
            DeleteObject(hBorderPen);

            // BitBlt back-buffer to screen
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, hMemDC, 0, 0, SRCCOPY);
            SelectObject(hMemDC, hOldBmp);
            DeleteObject(hMemBmp);
            DeleteDC(hMemDC);

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_NCDESTROY: {
            delete info;
            RemoveWindowSubclass(hWnd, ButtonSubclassProc, uIdSubclass);
            break;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void SubclassButton(HWND hWndButton, COLORREF normalColor, COLORREF hoverColor, COLORREF textColor) {
    ButtonSubclassInfo* info = new ButtonSubclassInfo{ normalColor, hoverColor, textColor, false, false };
    SetWindowSubclass(hWndButton, ButtonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(info));
}

// ==========================================
// Custom Seekbar Implementation
// ==========================================

struct SeekbarData {
    double position; // Current position in seconds
    double duration; // Max duration in seconds
    bool isDragging;
    bool isHovered;
};

LRESULT CALLBACK SeekbarWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SeekbarData* data = reinterpret_cast<SeekbarData*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (uMsg) {
        case WM_CREATE: {
            data = new SeekbarData{ 0.0, 0.0, false, false };
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
            break;
        }
        case SBM_SETPOSITION: {
            if (wParam == 1 && data->isDragging) break;
            data->position = static_cast<double>(lParam) / 1000.0;
            InvalidateRect(hWnd, NULL, FALSE);
            return 1;
        }
        case SBM_SETDURATION: {
            data->duration = static_cast<double>(lParam) / 1000.0;
            InvalidateRect(hWnd, NULL, FALSE);
            return 1;
        }
        case SBM_GETPOSITION: {
            return static_cast<LRESULT>(data->position * 1000.0);
        }
        case WM_MOUSEMOVE: {
            if (!data->isHovered) {
                data->isHovered = true;
                TRACKMOUSEEVENT tme = { sizeof(tme) };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, NULL, FALSE);
            }

            if (data->isDragging && data->duration > 0) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                int x = GET_X_LPARAM(lParam);
                int width = rc.right - 16; // 8px margin on each side
                
                double ratio = static_cast<double>(x - 8) / width;
                if (ratio < 0.0) ratio = 0.0;
                if (ratio > 1.0) ratio = 1.0;

                data->position = ratio * data->duration;
                InvalidateRect(hWnd, NULL, FALSE);

                // Notify parent that we are seeking
                HWND hParent = GetParent(hWnd);
                int id = GetDlgCtrlID(hWnd);
                SendMessage(hParent, WM_COMMAND, MAKEWPARAM(id, SBN_SEEKING), (LPARAM)hWnd);
            }
            break;
        }
        case WM_MOUSELEAVE: {
            data->isHovered = false;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        case WM_LBUTTONDOWN: {
            if (data->duration <= 0) break;
            SetCapture(hWnd);
            data->isDragging = true;

            RECT rc;
            GetClientRect(hWnd, &rc);
            int x = GET_X_LPARAM(lParam);
            int width = rc.right - 16;
            
            double ratio = static_cast<double>(x - 8) / width;
            if (ratio < 0.0) ratio = 0.0;
            if (ratio > 1.0) ratio = 1.0;

            data->position = ratio * data->duration;
            InvalidateRect(hWnd, NULL, FALSE);

            HWND hParent = GetParent(hWnd);
            int id = GetDlgCtrlID(hWnd);
            SendMessage(hParent, WM_COMMAND, MAKEWPARAM(id, SBN_SEEKING), (LPARAM)hWnd);
            break;
        }
        case WM_LBUTTONUP: {
            if (data->isDragging) {
                data->isDragging = false;
                ReleaseCapture();

                HWND hParent = GetParent(hWnd);
                int id = GetDlgCtrlID(hWnd);
                SendMessage(hParent, WM_COMMAND, MAKEWPARAM(id, SBN_SEEK_DONE), (LPARAM)hWnd);
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            // Double Buffering
            HDC hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HGDIOBJ hOldBmp = SelectObject(hMemDC, hMemBmp);

            // Draw Background
            HBRUSH hBgBrush = CreateSolidBrush(COLOR_BG);
            FillRect(hMemDC, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            // Track lines constants
            int trackY = rc.bottom / 2;
            int margin = 8;
            int trackWidth = rc.right - 2 * margin;

            // 1. Draw Gray Background Track Line
            HPEN hBgPen = CreatePen(PS_SOLID, data->isHovered ? 6 : 4, RGB(210, 214, 222));
            HGDIOBJ hOldPen = SelectObject(hMemDC, hBgPen);
            MoveToEx(hMemDC, margin, trackY, NULL);
            LineTo(hMemDC, rc.right - margin, trackY);
            SelectObject(hMemDC, hOldPen);
            DeleteObject(hBgPen);

            // 2. Draw Active Blue Track Line
            if (data->duration > 0) {
                double ratio = data->position / data->duration;
                int fillX = margin + static_cast<int>(ratio * trackWidth);

                HPEN hActivePen = CreatePen(PS_SOLID, data->isHovered ? 6 : 4, COLOR_ACCENT);
                hOldPen = SelectObject(hMemDC, hActivePen);
                MoveToEx(hMemDC, margin, trackY, NULL);
                LineTo(hMemDC, fillX, trackY);
                SelectObject(hMemDC, hOldPen);
                DeleteObject(hActivePen);

                // 3. Draw Thumb (Playhead)
                int thumbRadius = data->isHovered ? 8 : 6;
                HBRUSH hThumbBrush = CreateSolidBrush(COLOR_TEXT_WHITE);
                HGDIOBJ hOldBrush = SelectObject(hMemDC, hThumbBrush);

                HPEN hThumbPen = CreatePen(PS_SOLID, 2, COLOR_ACCENT);
                hOldPen = SelectObject(hMemDC, hThumbPen);

                Ellipse(hMemDC, fillX - thumbRadius, trackY - thumbRadius, fillX + thumbRadius, trackY + thumbRadius);

                SelectObject(hMemDC, hOldBrush);
                SelectObject(hMemDC, hOldPen);
                DeleteObject(hThumbBrush);
                DeleteObject(hThumbPen);
            }

            // BitBlt back-buffer to screen
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, hMemDC, 0, 0, SRCCOPY);
            SelectObject(hMemDC, hOldBmp);
            DeleteObject(hMemBmp);
            DeleteDC(hMemDC);

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_NCDESTROY: {
            delete data;
            break;
        }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// ==========================================
// Custom CutsList Implementation
// ==========================================

struct CutsListData {
    std::vector<VideoClip> clips;
    int scrollOffset;
    int hoverIndex;
    int hoverButton; // 0 = none, 1 = preview, 2 = delete
};

#define ROW_HEIGHT    55
#define PREVIEW_BTN_W 85
#define PREVIEW_BTN_H 30
#define DELETE_BTN_W  85
#define DELETE_BTN_H  30

LRESULT CALLBACK CutsListWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CutsListData* data = reinterpret_cast<CutsListData*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (uMsg) {
        case WM_CREATE: {
            data = new CutsListData{ std::vector<VideoClip>(), 0, -1, 0 };
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
            break;
        }
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            int oldHoverIndex = data->hoverIndex;
            int oldHoverButton = data->hoverButton;

            data->hoverIndex = -1;
            data->hoverButton = 0;

            RECT rc;
            GetClientRect(hWnd, &rc);
            int width = rc.right;

            for (size_t i = 0; i < data->clips.size(); ++i) {
                int rowY = static_cast<int>(i) * ROW_HEIGHT - data->scrollOffset;
                if (y >= rowY && y < rowY + ROW_HEIGHT) {
                    data->hoverIndex = static_cast<int>(i);

                    // Preview Button boundaries
                    int btnPreviewX = width - DELETE_BTN_W - 15 - PREVIEW_BTN_W - 15;
                    int btnY = rowY + (ROW_HEIGHT - PREVIEW_BTN_H) / 2;
                    if (x >= btnPreviewX && x < btnPreviewX + PREVIEW_BTN_W && y >= btnY && y < btnY + PREVIEW_BTN_H) {
                        data->hoverButton = 1;
                    }
                    // Delete Button boundaries
                    int btnDeleteX = width - DELETE_BTN_W - 15;
                    if (x >= btnDeleteX && x < btnDeleteX + DELETE_BTN_W && y >= btnY && y < btnY + DELETE_BTN_H) {
                        data->hoverButton = 2;
                    }
                    break;
                }
            }

            if (data->hoverIndex != oldHoverIndex || data->hoverButton != oldHoverButton) {
                TRACKMOUSEEVENT tme = { sizeof(tme) };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        }
        case WM_MOUSELEAVE: {
            data->hoverIndex = -1;
            data->hoverButton = 0;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            RECT rc;
            GetClientRect(hWnd, &rc);
            int width = rc.right;

            for (size_t i = 0; i < data->clips.size(); ++i) {
                int rowY = static_cast<int>(i) * ROW_HEIGHT - data->scrollOffset;
                if (y >= rowY && y < rowY + ROW_HEIGHT) {
                    int btnPreviewX = width - DELETE_BTN_W - 15 - PREVIEW_BTN_W - 15;
                    int btnY = rowY + (ROW_HEIGHT - PREVIEW_BTN_H) / 2;

                    // Clicked Preview
                    if (x >= btnPreviewX && x < btnPreviewX + PREVIEW_BTN_W && y >= btnY && y < btnY + PREVIEW_BTN_H) {
                        HWND hParent = GetParent(hWnd);
                        SendMessage(hParent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), CLN_PREVIEW_CLIP), (LPARAM)i);
                    }
                    // Clicked Delete
                    int btnDeleteX = width - DELETE_BTN_W - 15;
                    if (x >= btnDeleteX && x < btnDeleteX + DELETE_BTN_W && y >= btnY && y < btnY + DELETE_BTN_H) {
                        HWND hParent = GetParent(hWnd);
                        SendMessage(hParent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), CLN_DELETE_CLIP), (LPARAM)i);
                    }
                    break;
                }
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            int width = rc.right;

            // Double Buffering
            HDC hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HGDIOBJ hOldBmp = SelectObject(hMemDC, hMemBmp);

            // Draw Background
            HBRUSH hBgBrush = CreateSolidBrush(COLOR_BG);
            FillRect(hMemDC, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            // Draw list elements
            SetBkMode(hMemDC, TRANSPARENT);
            SelectObject(hMemDC, g_Theme.hFontNormal);

            for (size_t i = 0; i < data->clips.size(); ++i) {
                int rowY = static_cast<int>(i) * ROW_HEIGHT - data->scrollOffset;

                // Draw Row Background card
                RECT rowRc = { 5, rowY + 3, width - 5, rowY + ROW_HEIGHT - 3 };
                HBRUSH hCardBrush = CreateSolidBrush(COLOR_CARD);
                FillRect(hMemDC, &rowRc, hCardBrush);
                DeleteObject(hCardBrush);

                // Highlight border if hovered
                HPEN hBorderPen = CreatePen(PS_SOLID, 1, (data->hoverIndex == static_cast<int>(i)) ? COLOR_ACCENT : COLOR_BORDER);
                HGDIOBJ hOldPen = SelectObject(hMemDC, hBorderPen);
                
                // Draw rounded look using Win32 API RoundRect
                HGDIOBJ hOldBrush = SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
                RoundRect(hMemDC, rowRc.left, rowRc.top, rowRc.right, rowRc.bottom, 6, 6);
                SelectObject(hMemDC, hOldBrush);
                SelectObject(hMemDC, hOldPen);
                DeleteObject(hBorderPen);

                // Draw Clip Info Texts
                std::string clipNum = "Clip #" + std::to_string(i + 1);
                std::wstring clipNumW(clipNum.begin(), clipNum.end());
                RECT textNumRc = { rowRc.left + 15, rowY, rowRc.left + 100, rowY + ROW_HEIGHT };
                SetTextColor(hMemDC, COLOR_TEXT_DARK);
                DrawTextW(hMemDC, clipNumW.c_str(), -1, &textNumRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                std::string startEnd = "Start: " + data->clips[i].startTime + "   →   End: " + data->clips[i].endTime;
                std::wstring startEndW(startEnd.begin(), startEnd.end());
                RECT textTimeRc = { rowRc.left + 105, rowY, width - 230, rowY + ROW_HEIGHT };
                SetTextColor(hMemDC, COLOR_TEXT_MUTED);
                DrawTextW(hMemDC, startEndW.c_str(), -1, &textTimeRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Draw Row Action Buttons
                int btnPreviewX = width - DELETE_BTN_W - 15 - PREVIEW_BTN_W - 15;
                int btnY = rowY + (ROW_HEIGHT - PREVIEW_BTN_H) / 2;

                // 1. Draw "Preview" Button inside Row
                bool isPreviewHovered = (data->hoverIndex == static_cast<int>(i) && data->hoverButton == 1);
                HBRUSH hPrvBrush = CreateSolidBrush(isPreviewHovered ? COLOR_ACCENT_HOVER : COLOR_ACCENT);
                HPEN hPrvPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
                hOldBrush = SelectObject(hMemDC, hPrvBrush);
                hOldPen = SelectObject(hMemDC, hPrvPen);
                
                RoundRect(hMemDC, btnPreviewX, btnY, btnPreviewX + PREVIEW_BTN_W, btnY + PREVIEW_BTN_H, 6, 6);
                
                SetTextColor(hMemDC, COLOR_TEXT_WHITE);
                RECT btnPreviewRc = { btnPreviewX, btnY, btnPreviewX + PREVIEW_BTN_W, btnY + PREVIEW_BTN_H };
                DrawTextW(hMemDC, L"Preview", -1, &btnPreviewRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hMemDC, hOldBrush);
                SelectObject(hMemDC, hOldPen);
                DeleteObject(hPrvBrush);
                DeleteObject(hPrvPen);

                // 2. Draw "Delete" Button inside Row
                int btnDeleteX = width - DELETE_BTN_W - 15;
                bool isDeleteHovered = (data->hoverIndex == static_cast<int>(i) && data->hoverButton == 2);
                HBRUSH hDelBrush = CreateSolidBrush(isDeleteHovered ? RGB(220, 50, 50) : COLOR_CUT_HIGHLIGHT);
                HPEN hDelPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
                hOldBrush = SelectObject(hMemDC, hDelBrush);
                hOldPen = SelectObject(hMemDC, hDelPen);

                RoundRect(hMemDC, btnDeleteX, btnY, btnDeleteX + DELETE_BTN_W, btnY + DELETE_BTN_H, 6, 6);

                SetTextColor(hMemDC, COLOR_TEXT_WHITE);
                RECT btnDeleteRc = { btnDeleteX, btnY, btnDeleteX + DELETE_BTN_W, btnY + DELETE_BTN_H };
                DrawTextW(hMemDC, L"Delete", -1, &btnDeleteRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hMemDC, hOldBrush);
                SelectObject(hMemDC, hOldPen);
                DeleteObject(hDelBrush);
                DeleteObject(hDelPen);
            }

            // BitBlt back-buffer to screen
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, hMemDC, 0, 0, SRCCOPY);
            SelectObject(hMemDC, hOldBmp);
            DeleteObject(hMemBmp);
            DeleteDC(hMemDC);

            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_USER + 1: {
            // Custom Message: Add Clip
            VideoClip* clip = reinterpret_cast<VideoClip*>(lParam);
            data->clips.push_back(*clip);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        case WM_USER + 2: {
            // Custom Message: Remove Clip
            int index = static_cast<int>(wParam);
            if (index >= 0 && index < static_cast<int>(data->clips.size())) {
                data->clips.erase(data->clips.begin() + index);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        }
        case WM_USER + 3: {
            // Custom Message: Clear Clips
            data->clips.clear();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }
        case WM_USER + 4: {
            // Custom Message: Get Clips Count
            return data->clips.size();
        }
        case WM_USER + 5: {
            // Custom Message: Get Clip At
            int index = static_cast<int>(wParam);
            VideoClip* outClip = reinterpret_cast<VideoClip*>(lParam);
            if (index >= 0 && index < static_cast<int>(data->clips.size())) {
                *outClip = data->clips[index];
                return 1;
            }
            return 0;
        }
        case WM_NCDESTROY: {
            delete data;
            break;
        }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// ==========================================
// Main Custom Control Registration API
// ==========================================

void RegisterCustomControls(HINSTANCE hInstance) {
    g_Theme.Initialize();

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = SeekbarWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_HAND);
    wc.lpszClassName = WC_CUSTOM_SEEKBAR;
    wc.hbrBackground = NULL; // Double buffered, no flashing
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);

    WNDCLASSW wc2 = { 0 };
    wc2.lpfnWndProc = CutsListWndProc;
    wc2.hInstance = hInstance;
    wc2.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc2.lpszClassName = WC_CUSTOM_CUTS_LIST;
    wc2.hbrBackground = NULL; // Double buffered, no flashing
    wc2.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc2);
}
