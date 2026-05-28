#define _CRT_SECURE_NO_WARNINGS
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' \
processorArchitecture='*' \
publicKeyToken='6595b64144ccf1df' \
language='*'\"")

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "comctl32.lib")

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <urlmon.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <iomanip>
#include <sstream>
#include "app_styles.h"
#include "custom_controls.h"
#include "video_player.h"
#include "resource.h"

// Global Handles
HWND g_hWndMain = nullptr;
HWND g_hWndPreview = nullptr;
HWND g_hWndSeekbar = nullptr;
HWND g_hWndElapsed = nullptr;
HWND g_hWndTotalTime = nullptr;
HWND g_hWndFileLabel = nullptr;

HWND g_hWndPlayPause = nullptr;
HWND g_hWndBack5 = nullptr;
HWND g_hWndBack3 = nullptr;
HWND g_hWndForward3 = nullptr;
HWND g_hWndForward5 = nullptr;
HWND g_hWndBrowse = nullptr;

HWND g_hWndSetStart = nullptr;
HWND g_hWndSetEnd = nullptr;
HWND g_hWndTxtStart = nullptr;
HWND g_hWndTxtEnd = nullptr;
HWND g_hWndChkKeyframes = nullptr;
HWND g_hWndAddCut = nullptr;

HWND g_hWndCutsList = nullptr;
HWND g_hWndCutAll = nullptr;

HWND g_hWndTooltip = nullptr;

VideoPlayer g_Player;
std::wstring g_InputPath = L"";
double g_DurationSeconds = 0.0;
double g_CurrentTime = 0.0;
ULONGLONG g_LastSeekTime = 0;
bool g_IsPlaying = false;
UINT_PTR g_PlaybackTimerId = 0;

// Subclass procedures declarations
LRESULT CALLBACK TxtTimeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// Helpers
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string str(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, NULL, NULL);
    str.resize(strlen(str.c_str()));
    return str;
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
    wstr.resize(wcslen(wstr.c_str()));
    return wstr;
}

std::wstring FormatTime(double totalSeconds) {
    int h = static_cast<int>(totalSeconds) / 3600;
    int m = (static_cast<int>(totalSeconds) % 3600) / 60;
    int s = static_cast<int>(totalSeconds) % 60;
    std::wstringstream ss;
    ss << std::setw(2) << std::setfill(L'0') << h << L":"
       << std::setw(2) << std::setfill(L'0') << m << L":"
       << std::setw(2) << std::setfill(L'0') << s;
    return ss.str();
}

double ParseTime(const std::wstring& timeStr) {
    int h = 0, m = 0, s = 0;
    wchar_t colon;
    std::wstringstream ss(timeStr);
    ss >> h >> colon >> m >> colon >> s;
    return h * 3600.0 + m * 60.0 + s;
}

void UpdateLabels() {
    std::wstring elapsedW = FormatTime(g_CurrentTime);
    std::wstring totalW = FormatTime(g_DurationSeconds);

    SetWindowTextW(g_hWndElapsed, elapsedW.c_str());
    SetWindowTextW(g_hWndTotalTime, totalW.c_str());

    // Update seekbar position (wParam = 1 means only if not dragging)
    SendMessageW(g_hWndSeekbar, SBM_SETPOSITION, 1, (LPARAM)(g_CurrentTime * 1000.0));
}

void SeekToTime(double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    if (seconds > g_DurationSeconds) seconds = g_DurationSeconds;

    g_CurrentTime = seconds;
    g_LastSeekTime = GetTickCount64();

    std::thread([seconds]() {
        g_Player.Seek(seconds);
    }).detach();
    
    SendMessageW(g_hWndSeekbar, SBM_SETPOSITION, 0, (LPARAM)(seconds * 1000.0));

    UpdateLabels();
}

void SeekRelative(double secs) {
    if (g_DurationSeconds <= 0) return;
    SeekToTime(g_CurrentTime + secs);
}

void StopPlayback() {
    if (g_PlaybackTimerId) {
        KillTimer(g_hWndMain, g_PlaybackTimerId);
        g_PlaybackTimerId = 0;
    }
    std::thread([]() {
        g_Player.Pause();
    }).detach();
    g_IsPlaying = false;
    SetWindowTextW(g_hWndPlayPause, L"▶ Play");
}

void StartPlayback() {
    if (!g_Player.HasMedia()) return;
    std::thread([]() {
        g_Player.Play();
    }).detach();
    g_IsPlaying = true;
    SetWindowTextW(g_hWndPlayPause, L"⏸ Pause");
    
    g_PlaybackTimerId = SetTimer(g_hWndMain, 1, 1000, NULL);
}

void TogglePlayPause() {
    if (g_IsPlaying) StopPlayback();
    else StartPlayback();
}

// Check paths for external tools
std::wstring CheckDependency(const std::wstring& toolName) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/"));

    // 1. Check in app base directory (exe's own folder - highest priority)
    std::wstring localPath = exeDir + L"\\" + toolName;
    if (GetFileAttributesW(localPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return localPath;
    }
    
    // 2. Check in SuperVideoCutter_plugins subfolder (download fallback)
    std::wstring pluginPath = exeDir + L"\\SuperVideoCutter_plugins\\" + toolName;
    if (GetFileAttributesW(pluginPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return pluginPath;
    }

    // 3. Check system path using native SearchPathW API
    wchar_t pathBuf[MAX_PATH];
    wchar_t* filePart;
    DWORD res = SearchPathW(NULL, toolName.c_str(), NULL, MAX_PATH, pathBuf, &filePart);
    if (res > 0 && res < MAX_PATH) {
        return std::wstring(pathBuf);
    }

    return L"";
}

bool CheckAllTools(std::wstring& ffmpegPath, std::wstring& ffprobePath, std::wstring& vlcPath) {
    ffmpegPath = CheckDependency(L"ffmpeg.exe");
    ffprobePath = CheckDependency(L"ffprobe.exe");
    vlcPath = CheckDependency(L"libvlc.dll");
    return (!ffmpegPath.empty() && !ffprobePath.empty() && !vlcPath.empty());
}

// BindStatusCallback COM Class to report active download progress natively
class BindStatusCallback : public IBindStatusCallback {
public:
    HWND hWndProgress;
    HWND hDlg;
    std::wstring m_itemName;

    BindStatusCallback(HWND progress, HWND dlg, const std::wstring& itemName)
        : hWndProgress(progress), hDlg(dlg), m_itemName(itemName) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IBindStatusCallback) {
            *ppv = static_cast<IBindStatusCallback*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return 1; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }

    STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding* pib) override { return S_OK; }
    STDMETHODIMP GetPriority(LONG* pnPriority) override { return S_OK; }
    STDMETHODIMP OnLowResource(DWORD reserved) override { return S_OK; }
    STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) override {
        if (ulProgressMax > 0) {
            int percent = static_cast<int>((ulProgress * 100ULL) / ulProgressMax);
            SendMessageW(hWndProgress, PBM_SETPOS, percent, 0);
            
            std::wstring label = L"Downloading " + m_itemName + L"... (" + std::to_wstring(percent) + L"%)";
            SetDlgItemTextW(hDlg, 1002, label.c_str());
        }
        return S_OK;
    }
    STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError) override { return S_OK; }
    STDMETHODIMP GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) override { return S_OK; }
    STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed) override { return S_OK; }
    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown* punk) override { return S_OK; }
};

// Background downloader procedure
void BackgroundDownloaderThread(HWND hWndProgress, HWND hDlg) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/"));

    std::wstring pluginsDir = exeDir + L"\\SuperVideoCutter_plugins";
    CreateDirectoryW(pluginsDir.c_str(), NULL);

    std::wstring arch = (sizeof(void*) == 8) ? L"win64" : L"win32";
    std::wstring vlcUrl = L"https://download.videolan.org/pub/videolan/vlc/3.0.20/" + arch + L"/vlc-3.0.20-" + arch + L".zip";
    std::wstring ffmpegUrl = (sizeof(void*) == 8)
        ? L"https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip"
        : L"https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip";

    // 1. Download FFmpeg if needed
    std::wstring ffmpegDest = pluginsDir + L"\\ffmpeg_temp.zip";
    bool ffmpegNeed = (GetFileAttributesW((pluginsDir + L"\\ffmpeg.exe").c_str()) == INVALID_FILE_ATTRIBUTES || 
                       GetFileAttributesW((pluginsDir + L"\\ffprobe.exe").c_str()) == INVALID_FILE_ATTRIBUTES);
    
    if (ffmpegNeed) {
        SendMessageW(hWndProgress, PBM_SETPOS, 0, 0);
        BindStatusCallback callback(hWndProgress, hDlg, L"FFmpeg");
        HRESULT hr = URLDownloadToFileW(NULL, ffmpegUrl.c_str(), ffmpegDest.c_str(), 0, &callback);
        if (FAILED(hr)) {
            MessageBoxW(hDlg, L"Failed to download FFmpeg.", L"Error", MB_OK | MB_ICONERROR);
            EndDialog(hDlg, IDCANCEL);
            return;
        }

        // Extract FFmpeg
        SetDlgItemTextW(hDlg, 1002, L"Extracting FFmpeg...");
        // Temporarily set marquee style during extraction
        SetWindowLongPtrW(hWndProgress, GWL_STYLE, GetWindowLongPtrW(hWndProgress, GWL_STYLE) | PBS_MARQUEE);
        SetWindowPos(hWndProgress, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        SendMessageW(hWndProgress, PBM_SETMARQUEE, TRUE, 0);
        
        std::wstring script = L"Powershell -NoProfile -ExecutionPolicy Bypass -Command \""
            L"Add-Type -AssemblyName System.IO.Compression.FileSystem; "
            L"$zip = [System.IO.Compression.ZipFile]::OpenRead('" + pluginsDir + L"\\ffmpeg_temp.zip'); "
            L"ForEach ($e in $zip.Entries) { "
            L"  If ($e.Name -eq 'ffmpeg.exe' -or $e.Name -eq 'ffprobe.exe') { "
            L"    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, '" + pluginsDir + L"\\' + $e.Name, $true); "
            L"  } "
            L"} "
            L"$zip.Dispose(); "
            L"Remove-Item '" + pluginsDir + L"\\ffmpeg_temp.zip' -Force;\"";

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::vector<wchar_t> cmdBuf(script.begin(), script.end());
        cmdBuf.push_back(L'\0');

        // Using CREATE_NO_WINDOW completely eliminates CMD window flash
        if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    // 2. Download VLC if needed
    std::wstring vlcDest = pluginsDir + L"\\vlc_temp.zip";
    bool vlcNeed = (GetFileAttributesW((pluginsDir + L"\\libvlc.dll").c_str()) == INVALID_FILE_ATTRIBUTES);

    if (vlcNeed) {
        // Reset progress bar to standard mode (no marquee)
        SendMessageW(hWndProgress, PBM_SETMARQUEE, FALSE, 0);
        SetWindowLongPtrW(hWndProgress, GWL_STYLE, GetWindowLongPtrW(hWndProgress, GWL_STYLE) & ~PBS_MARQUEE);
        SetWindowPos(hWndProgress, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        SendMessageW(hWndProgress, PBM_SETPOS, 0, 0);

        BindStatusCallback callback(hWndProgress, hDlg, L"LibVLC");
        HRESULT hr = URLDownloadToFileW(NULL, vlcUrl.c_str(), vlcDest.c_str(), 0, &callback);
        if (FAILED(hr)) {
            MessageBoxW(hDlg, L"Failed to download LibVLC.", L"Error", MB_OK | MB_ICONERROR);
            EndDialog(hDlg, IDCANCEL);
            return;
        }

        // Extract VLC
        SetDlgItemTextW(hDlg, 1002, L"Extracting LibVLC...");
        SetWindowLongPtrW(hWndProgress, GWL_STYLE, GetWindowLongPtrW(hWndProgress, GWL_STYLE) | PBS_MARQUEE);
        SetWindowPos(hWndProgress, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        SendMessageW(hWndProgress, PBM_SETMARQUEE, TRUE, 0);

        std::wstring script = L"Powershell -NoProfile -ExecutionPolicy Bypass -Command \""
            L"Add-Type -AssemblyName System.IO.Compression.FileSystem; "
            L"$zip = [System.IO.Compression.ZipFile]::OpenRead('" + pluginsDir + L"\\vlc_temp.zip'); "
            L"$pluginsPath = New-Item -ItemType Directory -Force -Path '" + pluginsDir + L"\\plugins'; "
            L"ForEach ($e in $zip.Entries) { "
            L"  If ($e.Name -eq 'libvlc.dll' -or $e.Name -eq 'libvlccore.dll') { "
            L"    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, '" + pluginsDir + L"\\' + $e.Name, $true); "
            L"  } ElseIf ($e.FullName -match 'plugins/.*\\.dll$') { "
            L"    $subPath = $e.FullName.Substring($e.FullName.IndexOf('plugins/')); "
            L"    $dest = [System.IO.Path]::Combine('" + pluginsDir + L"', $subPath); "
            L"    $parent = Split-Path $dest; "
            L"    If (!(Test-Path $parent)) { New-Item -ItemType Directory -Force -Path $parent }; "
            L"    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, $dest, $true); "
            L"  } "
            L"} "
            L"$zip.Dispose(); "
            L"Remove-Item '" + pluginsDir + L"\\vlc_temp.zip' -Force;\"";

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::vector<wchar_t> cmdBuf(script.begin(), script.end());
        cmdBuf.push_back(L'\0');

        // Using CREATE_NO_WINDOW completely eliminates CMD window flash
        if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    MessageBoxW(hDlg, L"All dependencies successfully downloaded and extracted!", L"Success", MB_OK | MB_ICONINFORMATION);
    EndDialog(hDlg, IDOK);
}

// Dialog Procedure for Progress Dialog
INT_PTR CALLBACK ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            HWND hWndProgress = GetDlgItem(hDlg, 1001);
            // Ensure PBS_MARQUEE is NOT set initially so it behaves as a true progress bar
            SetWindowLongPtrW(hWndProgress, GWL_STYLE, GetWindowLongPtrW(hWndProgress, GWL_STYLE) & ~PBS_MARQUEE);
            SetWindowPos(hWndProgress, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            SendMessageW(hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessageW(hWndProgress, PBM_SETPOS, 0, 0);

            std::thread(BackgroundDownloaderThread, hWndProgress, hDlg).detach();
            return TRUE;
        }
    }
    return FALSE;
}

void TriggerDependencyDownload() {
    #pragma pack(push, 2)
    struct MY_DLGTEMPLATE {
        DWORD style;
        DWORD dwExtendedStyle;
        WORD cDlgItems;
        short x;
        short y;
        short cx;
        short cy;
    };
    struct MY_DLGITEMTEMPLATE {
        DWORD style;
        DWORD dwExtendedStyle;
        short x;
        short y;
        short cx;
        short cy;
        WORD id;
    };
    #pragma pack(pop)

    BYTE buffer[2048] = { 0 };
    
    // 1. Setup dialog template
    MY_DLGTEMPLATE* t = (MY_DLGTEMPLATE*)buffer;
    t->style = WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
    t->dwExtendedStyle = 0;
    t->cDlgItems = 2;
    t->x = 0;
    t->y = 0;
    t->cx = 240;
    t->cy = 80;

    BYTE* p = buffer + sizeof(MY_DLGTEMPLATE);
    
    // Menu (WORD): 0 (no menu)
    *(WORD*)p = 0; p += 2;
    // Class (WORD): 0 (default dialog class)
    *(WORD*)p = 0; p += 2;
    // Title (Unicode string)
    std::wstring title = L"Downloading Dependencies";
    wcscpy((wchar_t*)p, title.c_str());
    p += (title.length() + 1) * sizeof(wchar_t);

    // DLGITEMTEMPLATEs must be aligned on DWORD (4-byte) boundaries
    p = (BYTE*)(((uintptr_t)p + 3) & ~3);

    // 2. Item 1: Label (Static)
    MY_DLGITEMTEMPLATE* item1 = (MY_DLGITEMTEMPLATE*)p;
    item1->style = WS_CHILD | WS_VISIBLE | SS_CENTER;
    item1->dwExtendedStyle = 0;
    item1->x = 10;
    item1->y = 15;
    item1->cx = 220;
    item1->cy = 15;
    item1->id = 1002; // IDC_PROGRESS_LABEL
    p += sizeof(MY_DLGITEMTEMPLATE);

    // Class: Static (0xFFFF followed by 0x0082)
    *(WORD*)p = 0xFFFF; p += 2;
    *(WORD*)p = 0x0082; p += 2;

    // Title: L"Downloading and extracting tools..."
    std::wstring labelText = L"Downloading and extracting tools...";
    wcscpy((wchar_t*)p, labelText.c_str());
    p += (labelText.length() + 1) * sizeof(wchar_t);

    // Creation extra data size (WORD): 0
    *(WORD*)p = 0; p += 2;

    // Align to 4-byte boundary
    p = (BYTE*)(((uintptr_t)p + 3) & ~3);

    // 3. Item 2: Progress Bar
    MY_DLGITEMTEMPLATE* item2 = (MY_DLGITEMTEMPLATE*)p;
    item2->style = WS_CHILD | WS_VISIBLE; // No marquee style initially
    item2->dwExtendedStyle = 0;
    item2->x = 20;
    item2->y = 40;
    item2->cx = 200;
    item2->cy = 15;
    item2->id = 1001; // IDC_PROGRESS_BAR
    p += sizeof(MY_DLGITEMTEMPLATE);

    // Class: Unicode string "msctls_progress32"
    std::wstring progressClass = L"msctls_progress32";
    wcscpy((wchar_t*)p, progressClass.c_str());
    p += (progressClass.length() + 1) * sizeof(wchar_t);

    // Title: L"" (empty string)
    *(wchar_t*)p = L'\0'; p += sizeof(wchar_t);

    // Creation extra data size (WORD): 0
    *(WORD*)p = 0; p += 2;

    DialogBoxIndirectParamW(GetModuleHandleW(NULL), (LPDLGTEMPLATE)buffer, g_hWndMain, ProgressDlgProc, 0);
}

// Load Video File
void LoadVideo(const std::wstring& path) {
    g_InputPath = path;
    std::wstring fileName = path.substr(path.find_last_of(L"\\/") + 1);
    SetWindowTextW(g_hWndFileLabel, fileName.c_str());

    // Load video and query duration entirely on a background thread to prevent Win32-VLC GUI deadlocks
    std::thread([path]() {
        std::string pathUtf8 = WStringToString(path);
        if (g_Player.OpenMedia(pathUtf8, g_hWndPreview)) {
            g_Player.Play();
            
            // Poll for duration up to 5 seconds (VLC stream analysis delay fallback)
            double dur = 0.0;
            for (int i = 0; i < 50; ++i) {
                Sleep(100);
                dur = g_Player.GetLength();
                if (dur > 0.0) {
                    break;
                }
            }
            
            double* durPtr = new double(dur);
            PostMessageW(g_hWndMain, WM_USER + 10, 0, reinterpret_cast<LPARAM>(durPtr));
        }
    }).detach();
}

// Threaded ffmpeg execution to avoid freezing GUI
void ProcessCutsThread(std::wstring ffmpegPath, std::wstring inputPath, std::vector<VideoClip> clips, bool forceKeyframes) {
    std::wstring sourceDir = inputPath.substr(0, inputPath.find_last_of(L"\\/"));
    std::wstring baseName = inputPath.substr(inputPath.find_last_of(L"\\/") + 1);
    std::wstring ext = baseName.substr(baseName.find_last_of(L"."));
    baseName = baseName.substr(0, baseName.find_last_of(L"."));

    std::wstring exeDir = ffmpegPath.substr(0, ffmpegPath.find_last_of(L"\\/"));

    for (const auto& clip : clips) {
        std::wstring startW = StringToWString(clip.startTime);
        std::wstring endW = StringToWString(clip.endTime);

        std::wstring startClean = startW;
        std::wstring endClean = endW;
        for (auto& c : startClean) if (c == L':') c = L'-';
        for (auto& c : endClean) if (c == L':') c = L'-';

        std::wstring output = sourceDir + L"\\" + baseName + L"_" + startClean + L"_" + endClean + ext;
        
        std::wstring args;
        if (forceKeyframes) {
            // Re-encode starting at cut point to ensure it starts with a clean independent I-frame
            args = L"-ss " + startW + L" -to " + endW + L" -i \"" + inputPath + L"\" -c:v libx264 -crf 18 -preset superfast -c:a copy -y \"" + output + L"\"";
        } else {
            // Lossless cut copy
            args = L"-ss " + startW + L" -to " + endW + L" -i \"" + inputPath + L"\" -c copy -y \"" + output + L"\"";
        }

        // Run FFmpeg
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::wstring ffmpegCmd = L"\"" + ffmpegPath + L"\" " + args;
        std::vector<wchar_t> cmdBuf(ffmpegCmd.begin(), ffmpegCmd.end());
        cmdBuf.push_back(L'\0');

        // Using CREATE_NO_WINDOW completely eliminates CMD window flash
        if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    PostMessageW(g_hWndMain, WM_USER + 11, 0, reinterpret_cast<LPARAM>(new std::wstring(sourceDir)));
}

// Layout resize engine
void PerformLayout(int clientWidth, int clientHeight) {
    if (!g_hWndPreview) return;

    // Top Preview Panel
    MoveWindow(g_hWndPreview, 0, 0, clientWidth, 400, TRUE);

    // Timeline Panel (y=400, h=65)
    MoveWindow(g_hWndElapsed, 5, 422, 60, 20, TRUE);
    MoveWindow(g_hWndSeekbar, 57, 417, clientWidth - 114, 30, TRUE);
    MoveWindow(g_hWndTotalTime, clientWidth - 65, 422, 60, 20, TRUE);

    // Grid Row 0 (Buttons -5s, -3s, Play, +3s, +5s, Browse)
    int btnW = (clientWidth - 20) / 6;
    MoveWindow(g_hWndBack5, 5, 452, btnW, 45, TRUE);
    MoveWindow(g_hWndBack3, 5 + btnW, 452, btnW, 45, TRUE);
    MoveWindow(g_hWndPlayPause, 5 + 2 * btnW, 452, btnW, 45, TRUE);
    MoveWindow(g_hWndForward3, 5 + 3 * btnW, 452, btnW, 45, TRUE);
    MoveWindow(g_hWndForward5, 5 + 4 * btnW, 452, btnW, 45, TRUE);
    MoveWindow(g_hWndBrowse, 5 + 5 * btnW, 452, btnW, 45, TRUE);

    // Grid Row 1 (Mark Start, Mark End, Checkbox, Add to Cut)
    int grW = (clientWidth - 20) / 4;
    MoveWindow(g_hWndSetStart, 5, 507, grW - 75, 45, TRUE);
    MoveWindow(g_hWndTxtStart, 5 + grW - 70, 519, 65, 22, TRUE);

    MoveWindow(g_hWndSetEnd, 5 + grW, 507, grW - 75, 45, TRUE);
    MoveWindow(g_hWndTxtEnd, 5 + grW + grW - 70, 519, 65, 22, TRUE);

    MoveWindow(g_hWndChkKeyframes, 5 + 2 * grW + 10, 510, grW - 20, 38, TRUE);
    MoveWindow(g_hWndAddCut, 5 + 3 * grW, 507, grW, 45, TRUE);

    // Subtitle File Name label above List View
    MoveWindow(g_hWndFileLabel, 10, 562, clientWidth - 20, 20, TRUE);

    // List View (Middle scroll panel)
    int listTop = 587;
    int listBottom = clientHeight - 80;
    MoveWindow(g_hWndCutsList, 5, listTop, clientWidth - 10, listBottom - listTop, TRUE);

    // Process button at the bottom
    MoveWindow(g_hWndCutAll, 0, clientHeight - 80, clientWidth, 80, TRUE);
}

// WndProc Main
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Initialize dependencies dynamically
            std::wstring ffmpegPath, ffprobePath, vlcPath;
            if (!CheckAllTools(ffmpegPath, ffprobePath, vlcPath)) {
                int res = MessageBoxW(hWnd, L"Some required tools (FFmpeg or LibVLC) are missing. Download them now?", L"Setup", MB_YESNO | MB_ICONQUESTION);
                if (res == IDYES) {
                    TriggerDependencyDownload();
                }
            }

            // Re-check
            CheckAllTools(ffmpegPath, ffprobePath, vlcPath);

            // Load libVLC DLL
            if (!vlcPath.empty()) {
                std::wstring vlcFolder = vlcPath.substr(0, vlcPath.find_last_of(L"\\/"));
                // Set DLL directory so LoadLibrary can find libvlccore.dll next to libvlc.dll
                SetDllDirectoryW(vlcFolder.c_str());
                if (g_Player.LoadDll(vlcFolder)) {
                    if (!g_Player.InitializeVLC()) {
                        MessageBoxW(hWnd, L"VLC engine loaded but failed to initialize.\nPlugins folder may be missing or corrupt.", L"VLC Error", MB_OK | MB_ICONWARNING);
                    }
                } else {
                    std::wstring msg = L"Failed to load libvlc.dll from:\n" + vlcFolder + L"\n\nVideo preview will not be available.";
                    MessageBoxW(hWnd, msg.c_str(), L"VLC Error", MB_OK | MB_ICONWARNING);
                }
            } else {
                MessageBoxW(hWnd, L"libvlc.dll not found.\nVideo preview will not be available.\nPlease download dependencies.", L"VLC Error", MB_OK | MB_ICONWARNING);
            }

            // Create Preview black container panel
            g_hWndPreview = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BLACKRECT, 0, 0, 850, 400, hWnd, (HMENU)IDC_PREVIEW_WINDOW, NULL, NULL);
            // Allow Drag & Drop on the preview container window!
            DragAcceptFiles(g_hWndPreview, TRUE);

            // Custom Seekbar
            g_hWndSeekbar = CreateWindowExW(0, WC_CUSTOM_SEEKBAR, L"", WS_CHILD | WS_VISIBLE, 57, 417, 550, 30, hWnd, (HMENU)IDC_SEEKBAR, NULL, NULL);

            // Time Labels (Consolas small font)
            g_hWndElapsed = CreateWindowExW(0, L"STATIC", L"00:00:00", WS_CHILD | WS_VISIBLE | SS_LEFT, 5, 422, 60, 20, hWnd, NULL, NULL, NULL);
            g_hWndTotalTime = CreateWindowExW(0, L"STATIC", L"00:00:00", WS_CHILD | WS_VISIBLE | SS_RIGHT, 650, 422, 60, 20, hWnd, NULL, NULL, NULL);

            // Control Grid row 0
            g_hWndBack5 = CreateWindowExW(0, L"BUTTON", L"⏪ Back 5s", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_BACK_5, NULL, NULL);
            g_hWndBack3 = CreateWindowExW(0, L"BUTTON", L"⏪ Back 3s", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_BACK_3, NULL, NULL);
            g_hWndPlayPause = CreateWindowExW(0, L"BUTTON", L"▶ Play", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_PLAY_PAUSE, NULL, NULL);
            g_hWndForward3 = CreateWindowExW(0, L"BUTTON", L"Forward 3s ⏩", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_FORWARD_3, NULL, NULL);
            g_hWndForward5 = CreateWindowExW(0, L"BUTTON", L"Forward 5s ⏩", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_FORWARD_5, NULL, NULL);
            g_hWndBrowse = CreateWindowExW(0, L"BUTTON", L"Browse", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_BROWSE, NULL, NULL);

            // Style buttons with elegant light mode theme
            SubclassButton(g_hWndBack5, COLOR_CARD, COLOR_BORDER, COLOR_TEXT_DARK);
            SubclassButton(g_hWndBack3, COLOR_CARD, COLOR_BORDER, COLOR_TEXT_DARK);
            SubclassButton(g_hWndPlayPause, COLOR_CARD, COLOR_BORDER, COLOR_ACCENT);
            SubclassButton(g_hWndForward3, COLOR_CARD, COLOR_BORDER, COLOR_TEXT_DARK);
            SubclassButton(g_hWndForward5, COLOR_CARD, COLOR_BORDER, COLOR_TEXT_DARK);
            SubclassButton(g_hWndBrowse, COLOR_CARD, COLOR_BORDER, COLOR_TEXT_DARK);

            // Control Grid row 1
            g_hWndSetStart = CreateWindowExW(0, L"BUTTON", L"Mark Start", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_SET_START, NULL, NULL);
            g_hWndTxtStart = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"00:00:00", WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL, 0, 0, 100, 100, hWnd, (HMENU)IDC_TXT_START, NULL, NULL);
            
            g_hWndSetEnd = CreateWindowExW(0, L"BUTTON", L"Mark End", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_SET_END, NULL, NULL);
            g_hWndTxtEnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"00:00:00", WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL, 0, 0, 100, 100, hWnd, (HMENU)IDC_TXT_END, NULL, NULL);

            g_hWndChkKeyframes = CreateWindowExW(0, L"BUTTON", L"Force Keyframes\nat Cuts", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE, 0, 0, 100, 100, hWnd, (HMENU)IDC_CHK_KEYFRAMES, NULL, NULL);
            g_hWndAddCut = CreateWindowExW(0, L"BUTTON", L"Add Cut", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_ADD_CUT, NULL, NULL);

            SubclassButton(g_hWndSetStart, COLOR_CARD, COLOR_BORDER, COLOR_TEXT_DARK);
            SubclassButton(g_hWndSetEnd, COLOR_CARD, COLOR_BORDER, COLOR_TEXT_DARK);
            SubclassButton(g_hWndAddCut, COLOR_ACCENT, COLOR_ACCENT_HOVER, COLOR_TEXT_WHITE);

            // Set typography fonts on labels and inputs
            SendMessageW(g_hWndElapsed, WM_SETFONT, (WPARAM)g_Theme.hFontSmall, TRUE);
            SendMessageW(g_hWndTotalTime, WM_SETFONT, (WPARAM)g_Theme.hFontSmall, TRUE);
            SendMessageW(g_hWndTxtStart, WM_SETFONT, (WPARAM)g_Theme.hFontNormal, TRUE);
            SendMessageW(g_hWndTxtEnd, WM_SETFONT, (WPARAM)g_Theme.hFontNormal, TRUE);
            SendMessageW(g_hWndChkKeyframes, WM_SETFONT, (WPARAM)g_Theme.hFontNormal, TRUE);

            // Subclass Edit Fields to style them dark natively
            SetWindowSubclass(g_hWndTxtStart, TxtTimeSubclassProc, 1, 0);
            SetWindowSubclass(g_hWndTxtEnd, TxtTimeSubclassProc, 2, 0);

            // Create Tooltip for Checkbox
            g_hWndTooltip = CreateWindowExW(NULL, TOOLTIPS_CLASS, NULL,
                WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                hWnd, NULL, NULL, NULL);
            SendMessageW(g_hWndTooltip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)450);
            
            TOOLINFOW toolInfo = { 0 };
            toolInfo.cbSize = sizeof(toolInfo);
            toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            toolInfo.hwnd = hWnd;
            toolInfo.uId = (UINT_PTR)g_hWndChkKeyframes;
            toolInfo.lpszText = const_cast<LPWSTR>(
                L"Ensures that every cut point becomes a clean keyframe (I-frame), allowing precise, glitch-free trimming.\n"
                L"This improves playback accuracy and compatibility by making each cut start independently."
            );
            SendMessageW(g_hWndTooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

            // Subtitle Filename Display Label
            g_hWndFileLabel = CreateWindowExW(0, L"STATIC", L"No video file loaded. Drag & drop a video file here or click Browse.", WS_CHILD | WS_VISIBLE, 10, 580, 800, 20, hWnd, NULL, NULL, NULL);
            SendMessageW(g_hWndFileLabel, WM_SETFONT, (WPARAM)g_Theme.hFontNormal, TRUE);

            // Custom Cuts Grid List View
            g_hWndCutsList = CreateWindowExW(0, WC_CUSTOM_CUTS_LIST, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 5, 605, 830, 200, hWnd, (HMENU)IDC_CUTS_LIST, NULL, NULL);

            // Process cuts button at the bottom
            g_hWndCutAll = CreateWindowExW(0, L"BUTTON", L"Process All Cuts", WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, (HMENU)IDC_BTN_CUT_ALL, NULL, NULL);
            SubclassButton(g_hWndCutAll, COLOR_PROCESS, COLOR_PROCESS_HOVER, COLOR_TEXT_WHITE);
            SendMessageW(g_hWndCutAll, WM_SETFONT, (WPARAM)g_Theme.hFontBold, TRUE);

            return 0;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;

            SetBkMode(hdc, TRANSPARENT);
            if (hwndStatic == g_hWndElapsed || hwndStatic == g_hWndTotalTime) {
                SetTextColor(hdc, COLOR_TEXT_MUTED);
                return (INT_PTR)g_Theme.hBrushBg;
            } else if (hwndStatic == g_hWndFileLabel) {
                SetTextColor(hdc, COLOR_TEXT_DARK);
                return (INT_PTR)g_Theme.hBrushBg;
            } else if (hwndStatic == g_hWndChkKeyframes) {
                SetTextColor(hdc, COLOR_TEXT_DARK);
                return (INT_PTR)g_Theme.hBrushBg;
            }
            return (INT_PTR)g_Theme.hBrushBg;
        }
        case WM_DROPFILES: {
            // Intercept file drops from child static control or main window
            HDROP hDrop = (HDROP)wParam;
            wchar_t filePath[MAX_PATH];
            if (DragQueryFileW(hDrop, 0, filePath, MAX_PATH)) {
                LoadVideo(filePath);
            }
            DragFinish(hDrop);
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == IDC_BTN_BROWSE && code == BN_CLICKED) {
                // Open File Dialog
                wchar_t fileBuf[MAX_PATH] = { 0 };
                OPENFILENAMEW ofn = { 0 };
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFilter = L"Video Files\0*.mp4;*.mkv;*.avi;*.mov\0All Files\0*.*\0";
                ofn.lpstrFile = fileBuf;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                if (GetOpenFileNameW(&ofn)) {
                    LoadVideo(fileBuf);
                }
            } else if (id == IDC_BTN_PLAY_PAUSE && code == BN_CLICKED) {
                TogglePlayPause();
            } else if (id == IDC_BTN_BACK_5 && code == BN_CLICKED) {
                SeekRelative(-5.0);
            } else if (id == IDC_BTN_BACK_3 && code == BN_CLICKED) {
                SeekRelative(-3.0);
            } else if (id == IDC_BTN_FORWARD_3 && code == BN_CLICKED) {
                SeekRelative(3.0);
            } else if (id == IDC_BTN_FORWARD_5 && code == BN_CLICKED) {
                SeekRelative(5.0);
            } else if (id == IDC_BTN_SET_START && code == BN_CLICKED) {
                wchar_t cur[64];
                GetWindowTextW(g_hWndElapsed, cur, 64);
                SetWindowTextW(g_hWndTxtStart, cur);
            } else if (id == IDC_BTN_SET_END && code == BN_CLICKED) {
                wchar_t cur[64];
                GetWindowTextW(g_hWndElapsed, cur, 64);
                SetWindowTextW(g_hWndTxtEnd, cur);
            } else if (id == IDC_BTN_ADD_CUT && code == BN_CLICKED) {
                wchar_t start[64], end[64];
                GetWindowTextW(g_hWndTxtStart, start, 64);
                GetWindowTextW(g_hWndTxtEnd, end, 64);

                VideoClip clip{ WStringToString(start), WStringToString(end) };
                SendMessageW(g_hWndCutsList, WM_USER + 1, 0, (LPARAM)&clip);
            } else if (id == IDC_SEEKBAR) {
                if (code == SBN_SEEKING) {
                    double seconds = static_cast<double>(SendMessageW(g_hWndSeekbar, SBM_GETPOSITION, 0, 0)) / 1000.0;
                    g_CurrentTime = seconds;
                    g_LastSeekTime = GetTickCount64();
                    UpdateLabels();
                } else if (code == SBN_SEEK_DONE) {
                    double seconds = static_cast<double>(SendMessageW(g_hWndSeekbar, SBM_GETPOSITION, 0, 0)) / 1000.0;
                    SeekToTime(seconds);
                }
            } else if (id == IDC_CUTS_LIST) {
                if (code == CLN_PREVIEW_CLIP) {
                    int index = static_cast<int>(lParam);
                    VideoClip clip;
                    if (SendMessageW(g_hWndCutsList, WM_USER + 5, index, (LPARAM)&clip)) {
                        double seconds = ParseTime(StringToWString(clip.startTime));
                        SeekToTime(seconds);
                    }
                } else if (code == CLN_DELETE_CLIP) {
                    int index = static_cast<int>(lParam);
                    SendMessageW(g_hWndCutsList, WM_USER + 2, index, 0);
                }
            } else if (id == IDC_BTN_CUT_ALL && code == BN_CLICKED) {
                if (g_InputPath.empty()) return 0;
                int count = static_cast<int>(SendMessageW(g_hWndCutsList, WM_USER + 4, 0, 0));
                if (count <= 0) return 0;

                std::wstring ffmpegPath = CheckDependency(L"ffmpeg.exe");
                if (ffmpegPath.empty()) {
                    MessageBoxW(hWnd, L"ffmpeg.exe not found! Processing aborted.", L"Error", MB_OK | MB_ICONERROR);
                    return 0;
                }

                // Gather clips
                std::vector<VideoClip> clips;
                for (int i = 0; i < count; ++i) {
                    VideoClip clip;
                    SendMessageW(g_hWndCutsList, WM_USER + 5, i, (LPARAM)&clip);
                    clips.push_back(clip);
                }

                // Disable process button
                EnableWindow(g_hWndCutAll, FALSE);
                SetWindowTextW(g_hWndCutAll, L"Processing Cuts... Please Wait");

                bool forceKeyframes = (SendMessageW(g_hWndChkKeyframes, BM_GETCHECK, 0, 0) == BST_CHECKED);

                // Run cuts on a background thread to keep GUI responsive
                std::thread(ProcessCutsThread, ffmpegPath, g_InputPath, clips, forceKeyframes).detach();
            }

            break;
        }
        case WM_TIMER: {
            if (g_IsPlaying) {
                std::thread([]() {
                    double current = g_Player.GetTime();
                    double* pTime = new double(current);
                    PostMessageW(g_hWndMain, WM_USER + 12, 0, reinterpret_cast<LPARAM>(pTime));
                }).detach();
            }
            break;
        }
        case WM_USER + 10: {
            // Video Duration Query finished
            double* durPtr = reinterpret_cast<double*>(lParam);
            if (durPtr) {
                g_DurationSeconds = *durPtr;
                delete durPtr;
            }
            
            // Set Seekbar ranges
            SendMessageW(g_hWndSeekbar, SBM_SETDURATION, 0, (LPARAM)(g_DurationSeconds * 1000.0));
            SendMessageW(g_hWndSeekbar, SBM_SETPOSITION, 0, 0);

            g_CurrentTime = 0.0;
            UpdateLabels();
            StartPlayback();
            break;
        }
        case WM_USER + 11: {
            // Cut processing finished
            std::wstring* sourceDir = reinterpret_cast<std::wstring*>(lParam);
            
            EnableWindow(g_hWndCutAll, TRUE);
            SetWindowTextW(g_hWndCutAll, L"Process All Cuts");

            // Done dialog
            int res = MessageBoxW(hWnd, L"All cuts successfully completed. Open output folder?", L"Done", MB_YESNO | MB_ICONINFORMATION);
            if (res == IDYES) {
                ShellExecuteW(NULL, L"open", L"explorer.exe", sourceDir->c_str(), NULL, SW_SHOWNORMAL);
            }

            delete sourceDir;
            break;
        }
        case WM_USER + 12: {
            double* pTime = reinterpret_cast<double*>(lParam);
            if (pTime) {
                double current = *pTime;
                delete pTime;
                
                if (GetTickCount64() - g_LastSeekTime > 800) {
                    g_CurrentTime = current;
                    UpdateLabels();
                    
                    if (g_CurrentTime >= g_DurationSeconds) {
                        StopPlayback();
                    }
                }
            }
            break;
        }
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            PerformLayout(width, height);
            return 0;
        }
        case WM_DESTROY: {
            g_Player.ReleaseVLC();
            g_Theme.Cleanup();
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// Subclass Procedure to Style Text Fields to Dark Mode
LRESULT CALLBACK TxtTimeSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            // Draw Dark Background for Textbox
            HBRUSH hBgBrush = CreateSolidBrush(COLOR_CARD);
            FillRect(hdc, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            // Draw border
            HPEN hPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
            HGDIOBJ hOldPen = SelectObject(hdc, hPen);
            HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 4, 4);
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);

            // Draw Edit field Text
            wchar_t text[64];
            GetWindowTextW(hWnd, text, 64);
            
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_TEXT_DARK);
            SelectObject(hdc, g_Theme.hFontNormal);

            RECT textRc = { rc.left + 5, rc.top, rc.right - 5, rc.bottom };
            DrawTextW(hdc, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

std::wstring GetPortableRunDir() {
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        std::wstring runDir = exePath;
        size_t lastSlash = runDir.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            runDir = runDir.substr(0, lastSlash);
            std::wstring lowerDir = runDir;
            for (auto& c : lowerDir) c = towlower(c);
            if (lowerDir.length() >= 22 && lowerDir.substr(lowerDir.length() - 22) == L"\\supervideocutter_data") {
                return runDir;
            } else {
                return runDir + L"\\SuperVideoCutter_Data";
            }
        }
    }
    return L"";
}

class DownloadCallback : public IBindStatusCallback {
public:
    DownloadCallback(HWND hWndProgress, HWND hWndText, const std::wstring& taskName)
        : m_hWndProgress(hWndProgress), m_hWndText(hWndText), m_taskName(taskName), m_refCount(1) {}

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == IID_IBindStatusCallback) {
            *ppvObject = static_cast<IBindStatusCallback*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_refCount); }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG ref = InterlockedDecrement(&m_refCount);
        if (ref == 0) {
            delete this;
            return 0;
        }
        return ref;
    }

    // IBindStatusCallback methods
    STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding* pib) override { return S_OK; }
    STDMETHODIMP GetPriority(LONG* pnPriority) override { return S_OK; }
    STDMETHODIMP OnLowResource(DWORD reserved) override { return S_OK; }
    STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) override {
        if (ulProgressMax > 0) {
            int percent = static_cast<int>((static_cast<double>(ulProgress) / ulProgressMax) * 100.0);
            SendMessageW(m_hWndProgress, PBM_SETPOS, percent, 0);
            
            wchar_t buf[256];
            swprintf_s(buf, L"%s: %d%% (%lu / %lu KB)", m_taskName.c_str(), percent, ulProgress / 1024, ulProgressMax / 1024);
            SetWindowTextW(m_hWndText, buf);
        } else {
            wchar_t buf[256];
            swprintf_s(buf, L"%s: %lu KB downloaded", m_taskName.c_str(), ulProgress / 1024);
            SetWindowTextW(m_hWndText, buf);
        }
        return S_OK;
    }
    STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError) override { return S_OK; }
    STDMETHODIMP GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) override { return S_OK; }
    STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed) override { return S_OK; }
    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown* punk) override { return S_OK; }

private:
    HWND m_hWndProgress;
    HWND m_hWndText;
    std::wstring m_taskName;
    LONG m_refCount;
};

HRESULT DownloadFileWithProgress(const std::wstring& url, const std::wstring& destPath, HWND hWndProgress, HWND hWndText, const std::wstring& taskName) {
    DownloadCallback* pCallback = new DownloadCallback(hWndProgress, hWndText, taskName);
    pCallback->AddRef();
    HRESULT hr = URLDownloadToFileW(NULL, url.c_str(), destPath.c_str(), 0, static_cast<IBindStatusCallback*>(pCallback));
    pCallback->Release();
    return hr;
}

bool ExtractZip(const std::wstring& zipPath, const std::wstring& targetDir, const std::wstring& arch) {
    std::wstring script = L"Powershell -NoProfile -ExecutionPolicy Bypass -Command \""
        L"Add-Type -AssemblyName System.IO.Compression.FileSystem; "
        L"$zip = [System.IO.Compression.ZipFile]::OpenRead('" + zipPath + L"'); "
        L"ForEach ($e in $zip.Entries) { "
        L"  If ($e.Name -eq 'libvlc.dll' -or $e.Name -eq 'libvlccore.dll' -or $e.Name -eq 'ffmpeg.exe' -or $e.Name -eq 'ffprobe.exe') { "
        L"    $dest = [System.IO.Path]::Combine('" + targetDir + L"', $e.Name); "
        L"    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, $dest, $true); "
        L"  } ElseIf ($e.FullName -match 'plugins/.*\\.dll$') { "
        L"    $subPath = $e.FullName.Substring($e.FullName.IndexOf('plugins/')); "
        L"    $dest = [System.IO.Path]::Combine('" + targetDir + L"', $subPath); "
        L"    $parent = Split-Path $dest; "
        L"    If (!(Test-Path $parent)) { New-Item -ItemType Directory -Force -Path $parent }; "
        L"    [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e, $dest, $true); "
        L"  } "
        L"} "
        L"$zip.Dispose(); "
        L"Remove-Item '" + zipPath + L"' -Force;\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> cmdBuf(script.begin(), script.end());
    cmdBuf.push_back(L'\0');

    if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

INT_PTR CALLBACK ExtractionDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            HWND hWndProgress = GetDlgItem(hDlg, 1001);
            HWND hWndText = GetDlgItem(hDlg, 1002);

            std::thread([hDlg, hWndProgress, hWndText]() {
                std::wstring runDir = GetPortableRunDir();
                
#ifdef _WIN64
                std::wstring vlcUrl = L"https://download.videolan.org/pub/videolan/vlc/3.0.20/win64/vlc-3.0.20-win64.zip";
                std::wstring arch = L"x64";
#else
                std::wstring vlcUrl = L"https://download.videolan.org/pub/videolan/vlc/3.0.20/win32/vlc-3.0.20-win32.zip";
                std::wstring arch = L"x86";
#endif
                std::wstring vlcZip = runDir + L"\\vlc.zip";
                
                // 1. Download VLC
                SetWindowTextW(hWndText, L"Downloading VLC player components...");
                HRESULT hr = DownloadFileWithProgress(vlcUrl, vlcZip, hWndProgress, hWndText, L"VLC Download");
                if (FAILED(hr)) {
                    MessageBoxW(hDlg, L"Failed to download VLC player files from official server. Please check your internet connection.", L"Download Error", MB_OK | MB_ICONERROR);
                    EndDialog(hDlg, IDCANCEL);
                    return;
                }

                // 2. Extract VLC
                SetWindowTextW(hWndText, L"Extracting VLC media components...");
                SendMessageW(hWndProgress, PBM_SETPOS, 0, 0);
                SetWindowLongPtrW(hWndProgress, GWL_STYLE, GetWindowLongPtrW(hWndProgress, GWL_STYLE) | PBS_MARQUEE);
                SetWindowPos(hWndProgress, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                SendMessageW(hWndProgress, PBM_SETMARQUEE, TRUE, 0);
                
                ExtractZip(vlcZip, runDir, arch);

                // Disable marquee for next download
                SendMessageW(hWndProgress, PBM_SETMARQUEE, FALSE, 0);
                SetWindowLongPtrW(hWndProgress, GWL_STYLE, GetWindowLongPtrW(hWndProgress, GWL_STYLE) & ~PBS_MARQUEE);
                SetWindowPos(hWndProgress, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                SendMessageW(hWndProgress, PBM_SETPOS, 0, 0);

                // 3. Download FFmpeg
                std::wstring ffmpegUrl = L"https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip";
                std::wstring ffmpegZip = runDir + L"\\ffmpeg.zip";
                SetWindowTextW(hWndText, L"Downloading FFmpeg components...");
                hr = DownloadFileWithProgress(ffmpegUrl, ffmpegZip, hWndProgress, hWndText, L"FFmpeg Download");
                if (FAILED(hr)) {
                    MessageBoxW(hDlg, L"Failed to download FFmpeg video files from official server. Please check your internet connection.", L"Download Error", MB_OK | MB_ICONERROR);
                    EndDialog(hDlg, IDCANCEL);
                    return;
                }

                // 4. Extract FFmpeg
                SetWindowTextW(hWndText, L"Extracting FFmpeg video components...");
                SendMessageW(hWndProgress, PBM_SETPOS, 0, 0);
                SetWindowLongPtrW(hWndProgress, GWL_STYLE, GetWindowLongPtrW(hWndProgress, GWL_STYLE) | PBS_MARQUEE);
                SetWindowPos(hWndProgress, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                SendMessageW(hWndProgress, PBM_SETMARQUEE, TRUE, 0);
                
                ExtractZip(ffmpegZip, runDir, arch);

                EndDialog(hDlg, IDOK);
            }).detach();
            return TRUE;
        }
    }
    return FALSE;
}

void LaunchRelaunchCopy(const std::wstring& runDir, const std::wstring& currentExe, LPWSTR lpCmdLine) {
    std::wstring targetExe = runDir + L"\\SuperVideoCutter.exe";
    
    // Copy current executable to runDir\SuperVideoCutter.exe
    CopyFileW(currentExe.c_str(), targetExe.c_str(), FALSE);
    
    // Launch the copy and exit
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    
    std::wstring cmd = L"\"" + targetExe + L"\" " + lpCmdLine;
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(L'\0');
    
    if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// Entry Point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring currentExe = exePath;
    std::wstring runDir = GetPortableRunDir();

    bool alreadyInRunDir = false;
    if (!runDir.empty()) {
        std::wstring lowerExe = currentExe;
        std::wstring lowerRunDir = runDir;
        for (auto& c : lowerExe) c = towlower(c);
        for (auto& c : lowerRunDir) c = towlower(c);
        if (lowerExe.find(lowerRunDir) == 0) {
            alreadyInRunDir = true;
        }
    }

    if (!alreadyInRunDir && !runDir.empty()) {
        // Create directory
        CreateDirectoryW(runDir.c_str(), NULL);
        
        std::wstring vlcDll = runDir + L"\\libvlc.dll";
        std::wstring ffmpegExe = runDir + L"\\ffmpeg.exe";
        std::wstring archFile = runDir + L"\\arch.txt";
        bool needExtract = (GetFileAttributesW(vlcDll.c_str()) == INVALID_FILE_ATTRIBUTES ||
                            GetFileAttributesW(ffmpegExe.c_str()) == INVALID_FILE_ATTRIBUTES);

        if (!needExtract) {
            HANDLE hFile = CreateFileW(archFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                char buf[16] = { 0 };
                DWORD read = 0;
                ReadFile(hFile, buf, 15, &read, NULL);
                CloseHandle(hFile);
                std::string existingArch(buf);
#ifdef _WIN64
                if (existingArch != "x64") needExtract = true;
#else
                if (existingArch != "x86") needExtract = true;
#endif
            } else {
                needExtract = true;
            }
        }

        if (needExtract) {
            // Prompt the user
            int result = MessageBoxW(NULL, 
                L"Dependencies (VLC & FFmpeg) are required to run SuperVideoCutter.\n\n"
                L"Would you like to download and extract them now? (approx. 75MB download, 180MB extracted)", 
                L"Download Dependencies", 
                MB_YESNO | MB_ICONQUESTION | MB_SYSTEMMODAL);
            if (result != IDYES) {
                return 0; // Terminate app if user chooses not to download
            }

            // Clean up old DLLs to prevent architecture mismatch issues
            DeleteFileW((runDir + L"\\libvlc.dll").c_str());
            DeleteFileW((runDir + L"\\libvlccore.dll").c_str());
            // Remove old plugins directory
            std::wstring delPluginsCmd = L"Powershell -NoProfile -ExecutionPolicy Bypass -Command \"Remove-Item -Recurse -Force -ErrorAction SilentlyContinue '" + runDir + L"\\plugins'\"";
            std::vector<wchar_t> delCmdBuf(delPluginsCmd.begin(), delPluginsCmd.end());
            delCmdBuf.push_back(L'\0');
            STARTUPINFOW delSi = { sizeof(delSi) };
            PROCESS_INFORMATION delPi = { 0 };
            delSi.dwFlags = STARTF_USESHOWWINDOW;
            delSi.wShowWindow = SW_HIDE;
            if (CreateProcessW(NULL, delCmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &delSi, &delPi)) {
                WaitForSingleObject(delPi.hProcess, 5000);
                CloseHandle(delPi.hProcess);
                CloseHandle(delPi.hThread);
            }

            // Show center-aligned extraction progress dialog
            #pragma pack(push, 2)
            struct MY_DLGTEMPLATE {
                DWORD style;
                DWORD dwExtendedStyle;
                WORD cDlgItems;
                short x;
                short y;
                short cx;
                short cy;
            };
            struct MY_DLGITEMTEMPLATE {
                DWORD style;
                DWORD dwExtendedStyle;
                short x;
                short y;
                short cx;
                short cy;
                WORD id;
            };
            #pragma pack(pop)

            BYTE buffer[2048] = { 0 };
            MY_DLGTEMPLATE* t = (MY_DLGTEMPLATE*)buffer;
            t->style = WS_POPUP | WS_BORDER | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
            t->dwExtendedStyle = 0;
            t->cDlgItems = 2;
            t->x = 0; t->y = 0; t->cx = 240; t->cy = 80;

            BYTE* p = buffer + sizeof(MY_DLGTEMPLATE);
            *(WORD*)p = 0; p += 2; // Menu
            *(WORD*)p = 0; p += 2; // Class
            std::wstring title = L"SuperVideoCutter Portable";
            wcscpy((wchar_t*)p, title.c_str());
            p += (title.length() + 1) * sizeof(wchar_t);

            p = (BYTE*)(((uintptr_t)p + 3) & ~3);

            // Item 1: Label
            MY_DLGITEMTEMPLATE* item1 = (MY_DLGITEMTEMPLATE*)p;
            item1->style = WS_CHILD | WS_VISIBLE | SS_CENTER;
            item1->dwExtendedStyle = 0;
            item1->x = 10; item1->y = 15; item1->cx = 220; item1->cy = 15;
            item1->id = 1002;
            p += sizeof(MY_DLGITEMTEMPLATE);
            *(WORD*)p = 0xFFFF; p += 2; *(WORD*)p = 0x0082; p += 2;
            std::wstring labelText = L"Setting up dependencies, please wait...";
            wcscpy((wchar_t*)p, labelText.c_str());
            p += (labelText.length() + 1) * sizeof(wchar_t);
            *(WORD*)p = 0; p += 2;

            p = (BYTE*)(((uintptr_t)p + 3) & ~3);

            // Item 2: Progress
            MY_DLGITEMTEMPLATE* item2 = (MY_DLGITEMTEMPLATE*)p;
            item2->style = WS_CHILD | WS_VISIBLE;
            item2->dwExtendedStyle = 0;
            item2->x = 20; item2->y = 40; item2->cx = 200; item2->cy = 15;
            item2->id = 1001;
            p += sizeof(MY_DLGITEMTEMPLATE);
            std::wstring progressClass = L"msctls_progress32";
            wcscpy((wchar_t*)p, progressClass.c_str());
            p += (progressClass.length() + 1) * sizeof(wchar_t);
            *(wchar_t*)p = L'\0'; p += sizeof(wchar_t);
            *(WORD*)p = 0; p += 2;

            INT_PTR dlgRes = DialogBoxIndirectParamW(GetModuleHandleW(NULL), (LPDLGTEMPLATE)buffer, NULL, ExtractionDlgProc, 0);
            if (dlgRes != IDOK) {
                // If download failed or was cancelled, cleanup and exit
                DeleteFileW((runDir + L"\\vlc.zip").c_str());
                DeleteFileW((runDir + L"\\ffmpeg.zip").c_str());
                return 0;
            }

            // Write our architecture to arch.txt
            HANDLE hFile = CreateFileW(archFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
#ifdef _WIN64
                WriteFile(hFile, "x64", 3, &written, NULL);
#else
                WriteFile(hFile, "x86", 3, &written, NULL);
#endif
                CloseHandle(hFile);
            }
        }

        // Copy ourselves and relaunch the copy from extracted directory
        LaunchRelaunchCopy(runDir, currentExe, lpCmdLine);
        return 0; // Terminate launcher process
    }

    // Initialize standard and progress bar common controls
    INITCOMMONCONTROLSEX icex = { 0 };
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    RegisterCustomControls(hInstance);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SuperVideoCutterMainClass";
    wc.hbrBackground = g_Theme.hBrushBg;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassW(&wc);

    g_hWndMain = CreateWindowExW(0, L"SuperVideoCutterMainClass", L"SuperVideoCutter",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 754, 854,
        NULL, NULL, hInstance, NULL);

    if (!g_hWndMain) return 0;

    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);

    // Global Pre-Filter Message Loop for global keyboard shortcuts (Space, Left, Right)
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            // Check if user is typing in the timestamp textboxes - in that case do NOT intercept keys!
            HWND hFocus = GetFocus();
            if (hFocus != g_hWndTxtStart && hFocus != g_hWndTxtEnd) {
                if (msg.wParam == VK_SPACE) {
                    SendMessageW(g_hWndMain, WM_COMMAND, IDC_BTN_PLAY_PAUSE, 0);
                    continue; // Skip standard dispatch
                } else if (msg.wParam == VK_LEFT) {
                    SeekRelative(-5.0);
                    continue; // Skip standard dispatch
                } else if (msg.wParam == VK_RIGHT) {
                    SeekRelative(5.0);
                    continue; // Skip standard dispatch
                }
            }
        } else if (msg.message == WM_DROPFILES) {
            // If dropping file on preview, handle it
            if (msg.hwnd == g_hWndPreview || msg.hwnd == g_hWndMain) {
                SendMessageW(g_hWndMain, WM_DROPFILES, msg.wParam, msg.lParam);
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
