#pragma once
#include <windows.h>
#include <string>

// LibVLC Types (opaque structures)
typedef struct libvlc_instance_t libvlc_instance_t;
typedef struct libvlc_media_player_t libvlc_media_player_t;
typedef struct libvlc_media_t libvlc_media_t;
typedef int64_t libvlc_time_t;

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    // Initialize/Load DLL dynamically from path
    bool LoadDll(const std::wstring& dllFolder);
    bool IsDllLoaded() const { return m_hDll != nullptr; }
    void UnloadDll();

    // Media Player Control
    bool InitializeVLC();
    void ReleaseVLC();
    bool IsVLCInitialized() const { return m_pVlc != nullptr; }

    bool OpenMedia(const std::string& filePath, HWND hWndVideo);
    void CloseMedia();
    bool HasMedia() const { return m_pMediaPlayer != nullptr; }

    void Play();
    void Pause();
    void Stop();
    bool IsPlaying();

    void Seek(double seconds);
    double GetTime(); // Elapsed time in seconds
    double GetLength(); // Total duration in seconds
    void SetVolume(int volume);

private:
    HMODULE m_hDll;
    libvlc_instance_t* m_pVlc;
    libvlc_media_player_t* m_pMediaPlayer;

    // Function pointers resolved from libvlc.dll
    typedef libvlc_instance_t* (*PFN_libvlc_new)(int argc, const char* const* argv);
    typedef void (*PFN_libvlc_release)(libvlc_instance_t* p_instance);
    typedef libvlc_media_player_t* (*PFN_libvlc_media_player_new)(libvlc_instance_t* p_libvlc_instance);
    typedef libvlc_media_player_t* (*PFN_libvlc_media_player_new_from_media)(libvlc_media_t* p_md);
    typedef void (*PFN_libvlc_media_player_release)(libvlc_media_player_t* p_mi);
    typedef void (*PFN_libvlc_media_player_set_hwnd)(libvlc_media_player_t* p_mi, void* drawable);
    typedef libvlc_media_t* (*PFN_libvlc_media_new_path)(libvlc_instance_t* p_instance, const char* path);
    typedef void (*PFN_libvlc_media_release)(libvlc_media_t* p_md);
    typedef int (*PFN_libvlc_media_player_play)(libvlc_media_player_t* p_mi);
    typedef void (*PFN_libvlc_media_player_pause)(libvlc_media_player_t* p_mi);
    typedef void (*PFN_libvlc_media_player_stop)(libvlc_media_player_t* p_mi);
    typedef int (*PFN_libvlc_media_player_is_playing)(libvlc_media_player_t* p_mi);
    typedef libvlc_time_t (*PFN_libvlc_media_player_get_time)(libvlc_media_player_t* p_mi);
    typedef void (*PFN_libvlc_media_player_set_time)(libvlc_media_player_t* p_mi, libvlc_time_t i_time);
    typedef libvlc_time_t (*PFN_libvlc_media_player_get_length)(libvlc_media_player_t* p_mi);

    PFN_libvlc_new m_fn_new;
    PFN_libvlc_release m_fn_release;
    PFN_libvlc_media_player_new m_fn_media_player_new;
    PFN_libvlc_media_player_new_from_media m_fn_media_player_new_from_media;
    PFN_libvlc_media_player_release m_fn_media_player_release;
    PFN_libvlc_media_player_set_hwnd m_fn_media_player_set_hwnd;
    PFN_libvlc_media_new_path m_fn_media_new_path;
    PFN_libvlc_media_release m_fn_media_release;
    PFN_libvlc_media_player_play m_fn_media_player_play;
    PFN_libvlc_media_player_pause m_fn_media_player_pause;
    PFN_libvlc_media_player_stop m_fn_media_player_stop;
    PFN_libvlc_media_player_is_playing m_fn_media_player_is_playing;
    PFN_libvlc_media_player_get_time m_fn_media_player_get_time;
    PFN_libvlc_media_player_set_time m_fn_media_player_set_time;
    PFN_libvlc_media_player_get_length m_fn_media_player_get_length;
};
