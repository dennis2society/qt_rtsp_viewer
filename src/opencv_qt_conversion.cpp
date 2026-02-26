#include "opencv_qt_conversion.hpp"
#include <iostream>

OpenCVQtConversion::OpenCVQtConversion()
{
}

cv::Mat OpenCVQtConversion::qImageToMat(const QImage &img)
{
    if (img.isNull())
        return {};

    if (img.format() == QImage::Format_BGR888) {
        return cv::Mat(img.height(), img.width(), CV_8UC3, const_cast<uchar *>(img.bits()),
                       img.bytesPerLine()).clone(); // deep copy (safe)
    }

    // Fallback
    QImage converted = img.convertToFormat(QImage::Format_BGR888);
    return cv::Mat(converted.height(), converted.width(), CV_8UC3, const_cast<uchar *>(converted.bits()), converted.bytesPerLine()).clone();
}

QImage OpenCVQtConversion::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

        return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step,
                      QImage::Format_RGB888).copy(); // deep copy
    } else if (mat.type() == CV_8UC4) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32).copy();
    }

    return {};
}

QImage OpenCVQtConversion::applyGaussBlur(const QImage &img, int kernelSize, double sigma)
{
    if (img.isNull())
        return {};

    // Ensure odd kernel size (required by OpenCV)
    if (kernelSize % 2 == 0)
        kernelSize += 1;

    // Ensure correct format
    cv::Mat src = qImageToMat(img);
    if (src.empty())
        return {};

    cv::Mat blurred;
    cv::GaussianBlur(src, blurred, cv::Size(kernelSize, kernelSize), sigma);

    // Wrap result back into QImage
    return QImage(blurred.data, blurred.cols, blurred.rows, blurred.step,
                  QImage::Format_BGR888).copy(); // deep copy to detach from cv::Mat memory
}
