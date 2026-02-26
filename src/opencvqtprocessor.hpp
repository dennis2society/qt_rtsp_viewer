#pragma once
#include <QImage>
#include <opencv2/opencv.hpp>

class OpenCVQtProcessor
{
public:
    OpenCVQtProcessor();
    static cv::Mat qImageToMat(const QImage &img);
    static QImage matToQImage(const cv::Mat &mat);
    static QImage applyGaussBlur(const QImage &img, int kernelSize = 10, double sigma = 1.5);
    static QImage applyBilateralFilter(const QImage &img, int diameter, double sigmaColor, double sigmaSpace);
    static QImage applyMotionDetectionOverlay(const QImage &currentFrame, const QImage &previousFrame, int sensitivity);
    static QImage applyMotionVectorsOverlay(const QImage &currentFrame, const QImage &previousFrame);
};
