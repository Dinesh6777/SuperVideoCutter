#include "video_player.h"
#include <iostream>
#include <vector>

VideoPlayer::VideoPlayer()
    : m_hDll(nullptr), m_pVlc(nullptr), m_pMediaPlayer(nullptr),
      m_fn_new(nullptr), m_fn_release(nullptr), m_fn_media_player_new(nullptr),
      m_fn_media_player_new_from_media(nullptr), m_fn_media_player_release(nullptr),
      m_fn_media_player_set_hwnd(nullptr), m_fn_media_new_path(nullptr),
      m_fn_media_release(nullptr), m_fn_media_player_play(nullptr),
      m_fn_media_player_pause(nullptr), m_fn_media_player_stop(nullptr),
      m_fn_media_player_is_playing(nullptr), m_fn_media_player_get_time(nullptr),
      m_fn_media_player_set_time(nullptr), m_fn_media_player_get_length(nullptr) {}

VideoPlayer::~VideoPlayer() {
    CloseMedia();
    ReleaseVLC();
    UnloadDll();
}

bool VideoPlayer::LoadDll(const std::wstring& dllFolder) {
    UnloadDll();

    std::wstring dllPath = dllFolder + L"\\libvlc.dll";
    
    // Set DLL directory so libvlccore.dll (implicit dependency of libvlc.dll) is found
    SetDllDirectoryW(dllFolder.c_str());
    
    // Crucial: LOAD_WITH_ALTERED_SEARCH_PATH allows libvlc.dll to load libvlccore.dll from its same folder
    m_hDll = LoadLibraryExW(dllPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!m_hDll) {
        // Try fallback in app directory
        m_hDll = LoadLibraryExW(L"libvlc.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

    if (!m_hDll) return false;

    // Resolve functions
    m_fn_new = (PFN_libvlc_new)GetProcAddress(m_hDll, "libvlc_new");
    m_fn_release = (PFN_libvlc_release)GetProcAddress(m_hDll, "libvlc_release");
    m_fn_media_player_new = (PFN_libvlc_media_player_new)GetProcAddress(m_hDll, "libvlc_media_player_new");
    m_fn_media_player_new_from_media = (PFN_libvlc_media_player_new_from_media)GetProcAddress(m_hDll, "libvlc_media_player_new_from_media");
    m_fn_media_player_release = (PFN_libvlc_media_player_release)GetProcAddress(m_hDll, "libvlc_media_player_release");
    m_fn_media_player_set_hwnd = (PFN_libvlc_media_player_set_hwnd)GetProcAddress(m_hDll, "libvlc_media_player_set_hwnd");
    m_fn_media_new_path = (PFN_libvlc_media_new_path)GetProcAddress(m_hDll, "libvlc_media_new_path");
    m_fn_media_release = (PFN_libvlc_media_release)GetProcAddress(m_hDll, "libvlc_media_release");
    m_fn_media_player_play = (PFN_libvlc_media_player_play)GetProcAddress(m_hDll, "libvlc_media_player_play");
    m_fn_media_player_pause = (PFN_libvlc_media_player_pause)GetProcAddress(m_hDll, "libvlc_media_player_pause");
    m_fn_media_player_stop = (PFN_libvlc_media_player_stop)GetProcAddress(m_hDll, "libvlc_media_player_stop");
    m_fn_media_player_is_playing = (PFN_libvlc_media_player_is_playing)GetProcAddress(m_hDll, "libvlc_media_player_is_playing");
    m_fn_media_player_get_time = (PFN_libvlc_media_player_get_time)GetProcAddress(m_hDll, "libvlc_media_player_get_time");
    m_fn_media_player_set_time = (PFN_libvlc_media_player_set_time)GetProcAddress(m_hDll, "libvlc_media_player_set_time");
    m_fn_media_player_get_length = (PFN_libvlc_media_player_get_length)GetProcAddress(m_hDll, "libvlc_media_player_get_length");

    if (!m_fn_new || !m_fn_release || !m_fn_media_player_new || !m_fn_media_player_release || 
        !m_fn_media_player_set_hwnd || !m_fn_media_new_path || !m_fn_media_release || 
        !m_fn_media_player_play || !m_fn_media_player_pause || !m_fn_media_player_stop || 
        !m_fn_media_player_is_playing || !m_fn_media_player_get_time || !m_fn_media_player_set_time || 
        !m_fn_media_player_get_length) {
        
        UnloadDll();
        return false;
    }

    return true;
}

void VideoPlayer::UnloadDll() {
    if (m_hDll) {
        FreeLibrary(m_hDll);
        m_hDll = nullptr;
    }
    m_fn_new = nullptr;
    m_fn_release = nullptr;
    m_fn_media_player_new = nullptr;
    m_fn_media_player_new_from_media = nullptr;
    m_fn_media_player_release = nullptr;
    m_fn_media_player_set_hwnd = nullptr;
    m_fn_media_new_path = nullptr;
    m_fn_media_release = nullptr;
    m_fn_media_player_play = nullptr;
    m_fn_media_player_pause = nullptr;
    m_fn_media_player_stop = nullptr;
    m_fn_media_player_is_playing = nullptr;
    m_fn_media_player_get_time = nullptr;
    m_fn_media_player_set_time = nullptr;
    m_fn_media_player_get_length = nullptr;
}

bool VideoPlayer::InitializeVLC() {
    ReleaseVLC();
    if (!m_hDll || !m_fn_new) return false;

    // Derive plugins path from where libvlc.dll was actually loaded
    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(m_hDll, dllPath, MAX_PATH);
    std::wstring dllDir = dllPath;
    size_t lastSlash = dllDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        dllDir = dllDir.substr(0, lastSlash);
    }

    // Check if plugins folder exists next to libvlc.dll (bundled mode)
    std::wstring pluginsW = dllDir + L"\\plugins";
    if (GetFileAttributesW(pluginsW.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // Fallback: check next to exe
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring exeDir = exePath;
        size_t exeSlash = exeDir.find_last_of(L"\\/");
        if (exeSlash != std::wstring::npos) {
            exeDir = exeDir.substr(0, exeSlash);
        }
        pluginsW = exeDir + L"\\plugins";
        if (GetFileAttributesW(pluginsW.c_str()) == INVALID_FILE_ATTRIBUTES) {
            // Last fallback: downloader plugins path
            pluginsW = exeDir + L"\\SuperVideoCutter_plugins\\plugins";
        }
    }
    
    // Set process environment variable to guarantee VLC core successfully locates all plugins!
    SetEnvironmentVariableW(L"VLC_PLUGIN_PATH", pluginsW.c_str());

    const char* const vlc_args[] = {
        "--no-video-title-show",
        "--no-stats",
        "--no-osd",
        "--quiet"
    };

    m_pVlc = m_fn_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    return m_pVlc != nullptr;
}

void VideoPlayer::ReleaseVLC() {
    CloseMedia();
    if (m_pVlc && m_fn_release) {
        m_fn_release(m_pVlc);
        m_pVlc = nullptr;
    }
}

bool VideoPlayer::OpenMedia(const std::string& filePath, HWND hWndVideo) {
    CloseMedia();
    if (!m_pVlc || !m_fn_media_new_path || !m_fn_media_player_new_from_media || !m_fn_media_player_set_hwnd) 
        return false;

    libvlc_media_t* pMedia = m_fn_media_new_path(m_pVlc, filePath.c_str());
    if (!pMedia) return false;

    m_pMediaPlayer = m_fn_media_player_new_from_media(pMedia);
    m_fn_media_release(pMedia); // VLC internal reference incremented, safe to release local pointer

    if (!m_pMediaPlayer) return false;

    // Direct libVLC to render into our HWND container
    m_fn_media_player_set_hwnd(m_pMediaPlayer, hWndVideo);

    return true;
}

void VideoPlayer::CloseMedia() {
    if (m_pMediaPlayer && m_fn_media_player_release) {
        if (m_fn_media_player_stop) {
            m_fn_media_player_stop(m_pMediaPlayer);
        }
        m_fn_media_player_release(m_pMediaPlayer);
        m_pMediaPlayer = nullptr;
    }
}

void VideoPlayer::Play() {
    if (m_pMediaPlayer && m_fn_media_player_play) {
        m_fn_media_player_play(m_pMediaPlayer);
    }
}

void VideoPlayer::Pause() {
    if (m_pMediaPlayer && m_fn_media_player_pause) {
        m_fn_media_player_pause(m_pMediaPlayer);
    }
}

void VideoPlayer::Stop() {
    if (m_pMediaPlayer && m_fn_media_player_stop) {
        m_fn_media_player_stop(m_pMediaPlayer);
    }
}

bool VideoPlayer::IsPlaying() {
    if (m_pMediaPlayer && m_fn_media_player_is_playing) {
        return m_fn_media_player_is_playing(m_pMediaPlayer) != 0;
    }
    return false;
}

void VideoPlayer::Seek(double seconds) {
    if (m_pMediaPlayer && m_fn_media_player_set_time) {
        libvlc_time_t ms = static_cast<libvlc_time_t>(seconds * 1000.0);
        m_fn_media_player_set_time(m_pMediaPlayer, ms);
    }
}

double VideoPlayer::GetTime() {
    if (m_pMediaPlayer && m_fn_media_player_get_time) {
        libvlc_time_t ms = m_fn_media_player_get_time(m_pMediaPlayer);
        return static_cast<double>(ms) / 1000.0;
    }
    return 0.0;
}

double VideoPlayer::GetLength() {
    if (m_pMediaPlayer && m_fn_media_player_get_length) {
        libvlc_time_t ms = m_fn_media_player_get_length(m_pMediaPlayer);
        return static_cast<double>(ms) / 1000.0;
    }
    return 0.0;
}
