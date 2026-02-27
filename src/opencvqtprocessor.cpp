#include "opencvqtprocessor.hpp"
#include <iostream>

OpenCVQtProcessor::OpenCVQtProcessor()
    : sourceMat(), workMat1(), workMat2(), workMat3(), rgbMat(), resultImage()
{
    // Buffers are lazy-allocated on first use to maintain flexibility
}

cv::Mat OpenCVQtProcessor::qImageToMat(const QImage &img)
{
    if (img.isNull())
        return {};

    if (img.format() == QImage::Format_BGR888) {
        sourceMat = cv::Mat(img.height(), img.width(), CV_8UC3, const_cast<uchar *>(img.bits()),
                            img.bytesPerLine()).clone(); // deep copy (safe)
        return sourceMat;
    }

    // Fallback - convert to BGR888 format
    QImage converted = img.convertToFormat(QImage::Format_BGR888);
    sourceMat = cv::Mat(converted.height(), converted.width(), CV_8UC3, const_cast<uchar *>(converted.bits()),
                        converted.bytesPerLine()).clone();
    return sourceMat;
}

QImage OpenCVQtProcessor::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3) {
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);

        resultImage = QImage(rgbMat.data, rgbMat.cols, rgbMat.rows, rgbMat.step,
                             QImage::Format_RGB888).copy(); // deep copy
        return resultImage;
    } else if (mat.type() == CV_8UC4) {
        resultImage = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32).copy();
        return resultImage;
    }

    return {};
}

QImage OpenCVQtProcessor::applyGaussBlur(const QImage &img, int kernelSize, double sigma)
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

    // Reuse workMat1 for the blurred result
    cv::GaussianBlur(src, workMat1, cv::Size(kernelSize, kernelSize), sigma);

    // Store in member and return copy
    resultImage = QImage(workMat1.data, workMat1.cols, workMat1.rows, workMat1.step,
                         QImage::Format_BGR888).copy();
    return resultImage;
}

QImage OpenCVQtProcessor::applyBilateralFilter(const QImage &img, int diameter, double sigmaColor, double sigmaSpace)
{
    if (img.isNull())
        return {};

    cv::Mat src = qImageToMat(img);
    if (src.empty())
        return {};

    // Reuse workMat1 for the filtered result
    cv::bilateralFilter(src, workMat1, diameter, sigmaColor, sigmaSpace);

    // Store in member and return copy
    resultImage = QImage(workMat1.data, workMat1.cols, workMat1.rows, workMat1.step,
                         QImage::Format_BGR888).copy();
    return resultImage;
}

QImage OpenCVQtProcessor::applyMotionDetectionOverlay(const QImage &currentFrame, const QImage &previousFrame, int sensitivity)
{
    if (currentFrame.isNull() || previousFrame.isNull())
        return {};

    cv::Mat currentMat = qImageToMat(currentFrame);
    cv::Mat previousMat = qImageToMat(previousFrame);

    if (currentMat.empty() || previousMat.empty())
        return {};

    // Reuse workMat1 and workMat2 for grayscale conversions
    cv::cvtColor(currentMat, workMat1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(previousMat, workMat2, cv::COLOR_BGR2GRAY);

    // Reuse workMat3 for diff and threshold operations
    cv::absdiff(workMat1, workMat2, workMat3);

    cv::Mat thresh;
    cv::threshold(workMat3, thresh, sensitivity, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Draw on a copy of currentMat
    cv::Mat resultMat = currentMat.clone();
    for (const auto &contour : contours) {
        if (cv::contourArea(contour) > 500) { // Filter small contours
            cv::Rect boundingBox = cv::boundingRect(contour);
            cv::rectangle(resultMat, boundingBox, cv::Scalar(0, 0, 255), 2); // Red rectangle
        }
    }

    // Store in member and return copy
    resultImage = QImage(resultMat.data, resultMat.cols, resultMat.rows, resultMat.step,
                         QImage::Format_BGR888).copy();
    return resultImage;
}

QImage OpenCVQtProcessor::applyMotionVectorsOverlay(const QImage &currentFrame, const QImage &previousFrame)
{
    if (currentFrame.isNull() || previousFrame.isNull())
        return currentFrame;

    cv::Mat currentMat = qImageToMat(currentFrame);
    cv::Mat previousMat = qImageToMat(previousFrame);

    if (currentMat.empty() || previousMat.empty())
        return currentFrame;

    // Convert to grayscale for optical flow calculation, reusing workMat1 and workMat2
    cv::cvtColor(currentMat, workMat1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(previousMat, workMat2, cv::COLOR_BGR2GRAY);

    // Downsample for faster computation (optical flow is expensive)
    float scale = 0.1f;
    cv::Mat grayCurrentSmall, grayPreviousSmall;
    cv::resize(workMat1, grayCurrentSmall, cv::Size(), scale, scale, cv::INTER_LINEAR);
    cv::resize(workMat2, grayPreviousSmall, cv::Size(), scale, scale, cv::INTER_LINEAR);

    // Calculate optical flow using Farneback method with optimized parameters
    cv::Mat flow;
    cv::calcOpticalFlowFarneback(grayPreviousSmall, grayCurrentSmall, flow, 0.5, 2, 9, 2, 5, 1.1, 0);

    // Draw motion vectors on the frame, reusing workMat3
    workMat3 = currentMat.clone();
    
    // Grid spacing for vector visualization (on downsampled space, then scale to original)
    int stepSize = 10;
    float arrowScale = 1.5f; // Scale factor for arrow length visualization
    
    for (int y = 0; y < flow.rows; y += stepSize) {
        for (int x = 0; x < flow.cols; x += stepSize) {
            cv::Point2f flowVec = flow.at<cv::Point2f>(y, x);
            
            // Calculate magnitude of motion vector
            float magnitude = std::sqrt(flowVec.x * flowVec.x + flowVec.y * flowVec.y);
            
            // Only draw if motion is significant
            if (magnitude > 0.2f) {
                // Scale coordinates back to original image
                int x_orig = static_cast<int>(x / scale);
                int y_orig = static_cast<int>(y / scale);
                
                // Starting point
                cv::Point p1(x_orig, y_orig);
                // Ending point (scale flow vector back to original dimensions)
                cv::Point p2(x_orig + static_cast<int>(flowVec.x * arrowScale / scale), 
                            y_orig + static_cast<int>(flowVec.y * arrowScale / scale));
                
                // Color based on magnitude (from blue to red)
                float normalizedMag = std::min(magnitude * 5.0f, 1.0f);
                uchar colorVal = static_cast<uchar>(normalizedMag * 255);
                cv::Scalar arrowColor(255 - colorVal, 128, colorVal); // BGR format
                
                // Draw arrow (simplified line instead of arrowedLine for performance)
                cv::line(workMat3, p1, p2, arrowColor, 2, cv::LINE_AA);
                // Draw small circle at start point
                cv::circle(workMat3, p1, 2, arrowColor, -1);
            }
        }
    }

    // Store in member and return copy
    resultImage = QImage(workMat3.data, workMat3.cols, workMat3.rows, workMat3.step,
                         QImage::Format_BGR888).copy();
    return resultImage;
}

