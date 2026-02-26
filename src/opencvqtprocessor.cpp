#include "opencvqtprocessor.hpp"
#include <iostream>

OpenCVQtProcessor::OpenCVQtProcessor()
{
}

cv::Mat OpenCVQtProcessor::qImageToMat(const QImage &img)
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

QImage OpenCVQtProcessor::matToQImage(const cv::Mat &mat)
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

    cv::Mat blurred;
    cv::GaussianBlur(src, blurred, cv::Size(kernelSize, kernelSize), sigma);

    // Wrap result back into QImage
    return QImage(blurred.data, blurred.cols, blurred.rows, blurred.step,
                  QImage::Format_BGR888).copy(); // deep copy to detach from cv::Mat memory
}

QImage OpenCVQtProcessor::applyBilateralFilter(const QImage &img, int diameter, double sigmaColor, double sigmaSpace)
{
    if (img.isNull())
        return {};

    cv::Mat src = qImageToMat(img);
    if (src.empty())
        return {};

    cv::Mat filtered;
    cv::bilateralFilter(src, filtered, diameter, sigmaColor, sigmaSpace);

    return QImage(filtered.data, filtered.cols, filtered.rows, filtered.step,
                  QImage::Format_BGR888).copy(); // deep copy to detach from cv::Mat memory
}

QImage OpenCVQtProcessor::applyMotionDetectionOverlay(const QImage &currentFrame, const QImage &previousFrame, int sensitivity)
{
    if (currentFrame.isNull() || previousFrame.isNull())
        return {};

    cv::Mat currentMat = qImageToMat(currentFrame);
    cv::Mat previousMat = qImageToMat(previousFrame);

    if (currentMat.empty() || previousMat.empty())
        return {};

    cv::Mat grayCurrent, grayPrevious;
    cv::cvtColor(currentMat, grayCurrent, cv::COLOR_BGR2GRAY);
    cv::cvtColor(previousMat, grayPrevious, cv::COLOR_BGR2GRAY);

    cv::Mat diff;
    cv::absdiff(grayCurrent, grayPrevious, diff);

    cv::Mat thresh;
    cv::threshold(diff, thresh, sensitivity, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto &contour : contours) {
        if (cv::contourArea(contour) > 500) { // Filter small contours
            cv::Rect boundingBox = cv::boundingRect(contour);
            cv::rectangle(currentMat, boundingBox, cv::Scalar(0, 0, 255), 2); // Red rectangle
        }
    }

    return QImage(currentMat.data, currentMat.cols, currentMat.rows, currentMat.step,
                  QImage::Format_BGR888).copy(); // deep copy to detach from cv::Mat memory
}

QImage OpenCVQtProcessor::applyMotionVectorsOverlay(const QImage &currentFrame, const QImage &previousFrame)
{
    if (currentFrame.isNull() || previousFrame.isNull())
        return currentFrame;

    cv::Mat currentMat = qImageToMat(currentFrame);
    cv::Mat previousMat = qImageToMat(previousFrame);

    if (currentMat.empty() || previousMat.empty())
        return currentFrame;

    // Convert to grayscale for optical flow calculation
    cv::Mat grayCurrent, grayPrevious;
    cv::cvtColor(currentMat, grayCurrent, cv::COLOR_BGR2GRAY);
    cv::cvtColor(previousMat, grayPrevious, cv::COLOR_BGR2GRAY);

    // Downsample for faster computation (optical flow is expensive)
    float scale = 0.1f;
    cv::Mat grayCurrentSmall, grayPreviousSmall;
    cv::resize(grayCurrent, grayCurrentSmall, cv::Size(), scale, scale, cv::INTER_LINEAR);
    cv::resize(grayPrevious, grayPreviousSmall, cv::Size(), scale, scale, cv::INTER_LINEAR);

    // Calculate optical flow using Farneback method with optimized parameters
    cv::Mat flow;
    cv::calcOpticalFlowFarneback(grayPreviousSmall, grayCurrentSmall, flow, 0.5, 2, 9, 2, 5, 1.1, 0);

    // Draw motion vectors on the frame
    cv::Mat resultMat = currentMat.clone();
    
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
                cv::line(resultMat, p1, p2, arrowColor, 2, cv::LINE_AA);
                // Draw small circle at start point
                cv::circle(resultMat, p1, 2, arrowColor, -1);
            }
        }
    }

    return QImage(resultMat.data, resultMat.cols, resultMat.rows, resultMat.step,
                  QImage::Format_BGR888).copy();
}

