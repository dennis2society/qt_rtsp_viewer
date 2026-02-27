#pragma once
#include <QFile>
#include <QImage>
#include <QElapsedTimer>
#include <deque>
#include <opencv2/opencv.hpp>

class OpenCVQtProcessor
{
public:
    OpenCVQtProcessor();
    ~OpenCVQtProcessor();
    
    cv::Mat qImageToMat(const QImage &img);
    QImage matToQImage(const cv::Mat &mat);
    QImage applyGaussBlur(const QImage &img, int kernelSize = 10, double sigma = 1.5);
    QImage applyBilateralFilter(const QImage &img, int diameter, double sigmaColor, double sigmaSpace);
    // Each overlay function takes:
    //   drawTarget  – the frame to draw decorations onto  (accumulating composite)
    //   clean*      – undecorated frame(s) used only for computation
    // This ensures no overlay's decorations are ever seen as motion by another.
    QImage applyMotionDetectionOverlay(const QImage &drawTarget,
                                       const QImage &cleanCurrent,
                                       const QImage &cleanPrevious,
                                       int sensitivity);
    QImage applyMotionVectorsOverlay(const QImage &drawTarget,
                                     const QImage &cleanCurrent,
                                     const QImage &cleanPrevious);
    QImage applyFaceDetection(const QImage &drawTarget, const QImage &cleanSource);

    // Returns fraction [0.0, 1.0] of motion between clean frames
    double computeMotionLevel(const QImage &cleanCurrent,
                              const QImage &cleanPrevious,
                              int sensitivity);
    // Draws a sliding-window motion bar chart onto drawTarget
    QImage applyMotionGraphOverlay(const QImage &drawTarget, double motionLevel);

    // Reset motion logging (clears log file and restarts timer)
    void resetMotionLog();

private:
    // Reusable buffers to reduce memory allocation overhead
    cv::Mat sourceMat;     // For qImageToMat conversions
    cv::Mat workMat1;      // For intermediate operations
    cv::Mat workMat2;      // For intermediate operations
    cv::Mat workMat3;      // For intermediate operations
    cv::Mat rgbMat;        // For RGB conversion in matToQImage
    QImage resultImage;    // For output image, reused across calls

    cv::CascadeClassifier faceCascade; // Haar cascade for face detection
    bool faceCascadeLoaded = false;

    // Motion graph history (sliding window)
    static constexpr int kMotionHistorySize = 120;
    std::deque<double> motionHistory;
    double smoothedMotionLevel = 0.0; // Exponentially smoothed motion level

    // Spike rejection: rolling window of raw values used to detect I-frame outliers
    static constexpr int kSpikeWindowSize = 30; // slightly larger than typical GOP (25)
    static constexpr double kSpikeFactor  = 3.0; // flag if raw > kSpikeFactor * window median
    std::deque<double> rawHistory;

    // Motion logging timer
    QElapsedTimer motionLogTimer;
    bool motionLogStarted = false;
    QFile csvFile; // CSV file for motion logging
};
