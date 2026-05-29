# SuperVideoCutter

**SuperVideoCutter** is a portable, high-performance, lossless multi cut video trimmer built. It is designed for speed and simplicity, allowing users to cut video segments without re-encoding, which preserves the original visual quality and processes files almost instantly.

## App preview
<img width="550" height="650" alt="image" src="https://github.com/user-attachments/assets/1e94ce2f-f194-46da-9b28-0ec29fd9c3ee" />


## [Download in Releases](https://github.com/Dinesh6777/SuperVideoCutter/releases)

## 🚀 Key Features

*   **Lossless Trimming**: Utilizes FFmpeg stream copying to cut videos without quality degradation.
*   **True Portability**: Compiles into a single-file executable with that requires no external files
*   **Self-Healing Dependencies**: Automatically checks for FFmpeg binaries in the system PATH, app folder, or a dedicated plugins folder.
*   **Auto-Downloader**: If FFmpeg tools are missing, the app features a built-in downloader with a progress bar to fetch and extract the required tools automatically.
*   **Interactive Timeline**: Supports point-and-click navigation on the video timeline for rapid seeking.
*   **High-DPI Support**: Optimized for modern displays with a fixed **850x950** layout and native DPI scaling.

## 🛠 Technical Specifications

*   **Target Framework**: .NET 10.0-windows
*   **Runtime Identifier**: win-x64
*   **Backend Dependencies**: ffmpeg.exe, ffplay.exe, ffprobe.exe
*   **Deployment Mode**: Self-contained or Framework-dependent portable
*   **UI Library**: Windows Forms (WinForms)

## 📖 Usage

1.  **Open Video**: Click **Browse Video** to select your source file.
2.  **Navigate**: Use the spacebar to play/pause or click directly on the timeline to seek to a specific time[cite: 13, 14].
3.  **Define Clips**: 
    *   Seek to your start point and click **Mark Start**.
    *   Seek to your end point and click **Mark End**.
    *   Click **Add to Cut** to queue the clip.
4.  **Export**: Click **PROCESS ALL LOSSLESS CUTS** to save all clips to the source folder.
    *   Clips are named using the format: `[OriginalName]_[StartTime]_[EndTime].[ext]`.

## 📦 Building the Portable App

To generate the single-file portable EXE with the embedded runtime and icon:

1.  Ensure `SVC_Appicon.ico` is in your project root.
2.  Run the following command:
    ```powershell
    dotnet publish -c Release -r win-x64 --self-contained true /p:PublishSingleFile=true
    ```

## 📜 License

This project is licensed under the **MIT License**.
```text
MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
