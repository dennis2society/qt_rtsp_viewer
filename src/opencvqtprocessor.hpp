#pragma once
#include <QImage>
#include <opencv2/opencv.hpp>

class OpenCVQtProcessor
{
public:
    OpenCVQtProcessor();
    
    cv::Mat qImageToMat(const QImage &img);
    QImage matToQImage(const cv::Mat &mat);
    QImage applyGaussBlur(const QImage &img, int kernelSize = 10, double sigma = 1.5);
    QImage applyBilateralFilter(const QImage &img, int diameter, double sigmaColor, double sigmaSpace);
    QImage applyMotionDetectionOverlay(const QImage &currentFrame, const QImage &previousFrame, int sensitivity);
    QImage applyMotionVectorsOverlay(const QImage &currentFrame, const QImage &previousFrame);
    QImage applyFaceDetection(const QImage &img);

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
};
