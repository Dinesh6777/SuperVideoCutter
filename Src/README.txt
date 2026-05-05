SUPER VIDEO CUTTER PRO (.NET 10)
================================

This project is a high-performance, lossless video segment cutter.

BUILD INSTRUCTIONS:
1. Ensure you have the .NET 10 SDK installed.
2. Open a terminal in this folder.
3. Run the following command to create a portable single-file EXE:
   dotnet publish -c Release -r win-x64 --self-contained true /p:PublishSingleFile=true /p:IncludeNativeLibrariesForSelfExtract=true

IMPORTANT - REQUIRED BINARIES:
For the app to work, you MUST place the following files in the same folder as your generated .exe:
1. ffmpeg.exe
2. ffplay.exe

You can download these from https://ffmpeg.org/download.html (gyan.dev windows builds).

HOW TO USE:
1. Browse for a video file.
2. It will preview in the top panel.
3. Enter start and end times (HH:MM:SS) and click "Add to Cut List".
4. Repeat for as many clips as you want.
5. Click "Process All Lossless Cuts" and choose an output folder.
6. The app will generate clips instantly without quality loss.
