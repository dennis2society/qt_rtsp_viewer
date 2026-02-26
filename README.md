# Qt RTSP Viewer

This is a simple Qt application to view an RTSP stream from the provided URL
with basic effects processing.

![Screenshot](qt_rtsp_viewer_screenshot.jpg)

## Dependencies
- Qt(6)
- OpenCV C++ dev libraries
- at least one RTSP camera with a known "rtsp://..." URL

## Build
Inside the cloned repo run:
```
cmake -B build -D CMAKE_BUILD_TYPE=Release
```
This should create the executable in ```./build/bin/```

## Transparency note
This application was partially developed with AI assistance (Claude Haiku/Sonnet 4.5, ChatGPT 4.1).
