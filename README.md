# Qt RTSP Viewer

This is a Qt6 application to view an RTSP stream from the provided URL
with basic effects processing, a motion detection playground and 
stream recording with ffmpeg (when available).

![Screenshot](qt_rtsp_viewer_screenshot.jpg)

## Dependencies
- Qt6
- OpenCV C++ dev libraries
- (optional) ffmpeg libraries to enable stream recording
- at least one RTSP camera with a known "rtsp://..." URL

## Build
Inside the cloned repo run:
```
cmake -B build -D CMAKE_BUILD_TYPE=Release
```
and then
```
cmake --build build
```
This should create the executable in ```./build/bin/```

## Transparency note
This application was developed with <b>heavy</b> AI assistance (Claude Haiku/Sonnet 4.5, ChatGPT 4.1)
using github Copilot.


2026, github (at) dennis2society.de