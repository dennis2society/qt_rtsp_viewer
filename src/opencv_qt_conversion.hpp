#pragma once
#include <QImage>
#include <opencv2/opencv.hpp>

class OpenCVQtConversion
{
public:
    OpenCVQtConversion();
    static cv::Mat qImageToMat(const QImage &img);
    static QImage matToQImage(const cv::Mat &mat);
    static QImage applyGaussBlur(const QImage &img, int kernelSize = 10, double sigma = 1.5);
};
