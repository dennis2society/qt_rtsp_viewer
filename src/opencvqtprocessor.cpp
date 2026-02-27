#include "opencvqtprocessor.hpp"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QTextStream>
#include <QDateTime>
#include <iostream>

const cv::String haarCascadePath = "opencv/haarcascade_frontalface_default.xml";

OpenCVQtProcessor::OpenCVQtProcessor()
    : sourceMat(), workMat1(), workMat2(), workMat3(), rgbMat(), resultImage(), csvFile("motion_log.csv")
{
  std::cout << "Opening csv file for motion logging: " << csvFile.fileName().toStdString() << std::endl;
  if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&csvFile);
    out << "Milliseconds,DateTime,MotionLevel%\n";
    std::cout << "Motion log CSV file initialized with header.\n";
  }
}

OpenCVQtProcessor::~OpenCVQtProcessor()
{
  std::cout<< "Closing csv file for motion logging: " << csvFile.fileName().toStdString() << std::endl;
    if (csvFile.isOpen()) {
        csvFile.close();
    }
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

QImage OpenCVQtProcessor::applyMotionDetectionOverlay(const QImage &drawTarget,
                                                       const QImage &cleanCurrent,
                                                       const QImage &cleanPrevious,
                                                       int sensitivity)
{
    if (cleanCurrent.isNull() || cleanPrevious.isNull() || drawTarget.isNull())
        return drawTarget;

    // Computation on clean frames only
    cv::Mat currentMat  = qImageToMat(cleanCurrent);
    cv::Mat previousMat = qImageToMat(cleanPrevious);
    if (currentMat.empty() || previousMat.empty())
        return drawTarget;

    cv::cvtColor(currentMat,  workMat1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(previousMat, workMat2, cv::COLOR_BGR2GRAY);
    cv::absdiff(workMat1, workMat2, workMat3);

    cv::Mat thresh;
    cv::threshold(workMat3, thresh, sensitivity, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Draw onto a copy of drawTarget (not cleanCurrent)
    cv::Mat canvas = qImageToMat(drawTarget).clone();
    for (const auto &contour : contours) {
        if (cv::contourArea(contour) > 500) {
            cv::Rect boundingBox = cv::boundingRect(contour);
            cv::rectangle(canvas, boundingBox, cv::Scalar(0, 0, 255), 2);
        }
    }

    resultImage = QImage(canvas.data, canvas.cols, canvas.rows, canvas.step,
                         QImage::Format_BGR888).copy();
    return resultImage;
}

QImage OpenCVQtProcessor::applyMotionVectorsOverlay(const QImage &drawTarget,
                                                     const QImage &cleanCurrent,
                                                     const QImage &cleanPrevious)
{
    if (cleanCurrent.isNull() || cleanPrevious.isNull() || drawTarget.isNull())
        return drawTarget;

    // Computation on clean frames only
    cv::Mat currentMat  = qImageToMat(cleanCurrent);
    cv::Mat previousMat = qImageToMat(cleanPrevious);
    if (currentMat.empty() || previousMat.empty())
        return drawTarget;

    cv::cvtColor(currentMat,  workMat1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(previousMat, workMat2, cv::COLOR_BGR2GRAY);

    float scale = 0.1f;
    cv::Mat grayCurrentSmall, grayPreviousSmall;
    cv::resize(workMat1, grayCurrentSmall,  cv::Size(), scale, scale, cv::INTER_LINEAR);
    cv::resize(workMat2, grayPreviousSmall, cv::Size(), scale, scale, cv::INTER_LINEAR);

    cv::Mat flow;
    cv::calcOpticalFlowFarneback(grayPreviousSmall, grayCurrentSmall,
                                 flow, 0.5, 2, 9, 2, 5, 1.1, 0);

    // Draw onto drawTarget (not cleanCurrent)
    cv::Mat canvas = qImageToMat(drawTarget).clone();

    int   stepSize  = 10;
    float arrowScale = 1.5f;
    for (int y = 0; y < flow.rows; y += stepSize) {
        for (int x = 0; x < flow.cols; x += stepSize) {
            cv::Point2f flowVec   = flow.at<cv::Point2f>(y, x);
            float       magnitude = std::sqrt(flowVec.x * flowVec.x + flowVec.y * flowVec.y);
            if (magnitude > 0.2f) {
                int x_orig = static_cast<int>(x / scale);
                int y_orig = static_cast<int>(y / scale);
                cv::Point p1(x_orig, y_orig);
                cv::Point p2(x_orig + static_cast<int>(flowVec.x * arrowScale / scale),
                             y_orig + static_cast<int>(flowVec.y * arrowScale / scale));
                float normalizedMag = std::min(magnitude * 5.0f, 1.0f);
                uchar colorVal      = static_cast<uchar>(normalizedMag * 255);
                cv::Scalar arrowColor(255 - colorVal, 128, colorVal);
                cv::line(canvas, p1, p2, arrowColor, 2, cv::LINE_AA);
                cv::circle(canvas, p1, 2, arrowColor, -1);
            }
        }
    }

    resultImage = QImage(canvas.data, canvas.cols, canvas.rows, canvas.step,
                         QImage::Format_BGR888).copy();
    return resultImage;
}

QImage OpenCVQtProcessor::applyFaceDetection(const QImage &drawTarget,
                                              const QImage &cleanSource)
{
    if (cleanSource.isNull() || drawTarget.isNull())
        return drawTarget;

    if (!faceCascadeLoaded)
        faceCascadeLoaded = faceCascade.load(haarCascadePath);
    if (!faceCascadeLoaded || faceCascade.empty())
        return drawTarget;

    // Detection on clean source
    cv::Mat src = qImageToMat(cleanSource);
    if (src.empty())
        return drawTarget;

    constexpr float scale = 0.5f;
    cv::Mat small;
    cv::resize(src, small, cv::Size(), scale, scale, cv::INTER_LINEAR);
    cv::Mat gray;
    cv::cvtColor(small, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.1, 4, 0, cv::Size(30, 30));

    // Draw onto drawTarget
    cv::Mat canvas = qImageToMat(drawTarget).clone();
    for (const cv::Rect &r : faces) {
        cv::Rect orig(static_cast<int>(r.x / scale),
                      static_cast<int>(r.y / scale),
                      static_cast<int>(r.width  / scale),
                      static_cast<int>(r.height / scale));
        cv::rectangle(canvas, orig, cv::Scalar(0, 255, 0), 2);
    }

    resultImage = QImage(canvas.data, canvas.cols, canvas.rows, canvas.step,
                         QImage::Format_BGR888).copy();
    return resultImage;
}

double OpenCVQtProcessor::computeMotionLevel(const QImage &currentFrame,
                                              const QImage &previousFrame,
                                              int /*sensitivity*/)
{
    if (currentFrame.isNull() || previousFrame.isNull())
        return 0.0;

    // Fully self-contained: convert locally without touching shared member buffers.
    auto toGray = [](const QImage &img) -> cv::Mat {
        QImage bgr = (img.format() == QImage::Format_BGR888)
                         ? img
                         : img.convertToFormat(QImage::Format_BGR888);
        cv::Mat mat(bgr.height(), bgr.width(), CV_8UC3,
                    const_cast<uchar *>(bgr.bits()),
                    static_cast<size_t>(bgr.bytesPerLine()));
        cv::Mat gray;
        cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
        return gray.clone(); // deep copy before bgr goes out of scope
    };

    cv::Mat grayA = toGray(currentFrame);
    cv::Mat grayB = toGray(previousFrame);
    if (grayA.empty() || grayB.empty())
        return 0.0;

    // Use mean absolute difference (range 0-255).
    // A mean diff of ~20 (≈8 % intensity change) is mapped to 1.0 on the chart.
    // This is far more sensitive than a threshold+pixel-count approach and
    // works independently of the sensitivity slider.
    cv::Mat diff;
    cv::absdiff(grayA, grayB, diff);
    double meanDiff = cv::mean(diff)[0]; // 0-255
    constexpr double kMaxDiff = 20.0;    // mean diff that saturates the chart
    return std::min(meanDiff / kMaxDiff, 1.0);
}

QImage OpenCVQtProcessor::applyMotionGraphOverlay(const QImage &img, double motionLevel)
{
    if (img.isNull())
        return img;

    // --- Spike rejection ---
    // Maintain a rolling window of raw values. If the new value is more than
    // kSpikeFactor times the window median it is almost certainly an I-frame
    // DC-refresh artefact, so we clamp it back to the median.
    rawHistory.push_back(motionLevel);
    while (static_cast<int>(rawHistory.size()) > kSpikeWindowSize)
        rawHistory.pop_front();

    double filteredLevel = motionLevel;
    if (static_cast<int>(rawHistory.size()) >= 5) {
        std::vector<double> sorted(rawHistory.begin(), rawHistory.end());
        std::sort(sorted.begin(), sorted.end());
        double median = sorted[sorted.size() / 2];
        if (median > 0.0 && motionLevel > kSpikeFactor * median)
            filteredLevel = median; // clamp spike to median
    }

    // Exponential smoothing on the spike-filtered value
    constexpr double SMOOTHING_FACTOR = 0.85;
    smoothedMotionLevel = SMOOTHING_FACTOR * smoothedMotionLevel +
                          (1.0 - SMOOTHING_FACTOR) * filteredLevel;

    // Log smoothed motion level to CSV
    QDateTime now = QDateTime::currentDateTime();
    qint64 msecsSinceEpoch = now.toMSecsSinceEpoch();
    QString timeStr = now.toString("yyyy-MM-dd HH:mm:ss");
    QString motionPercent = QString::number(smoothedMotionLevel * 100, 'f', 2);

    if (!csvFile.isOpen()) {
        if (!csvFile.open(QIODevice::Append | QIODevice::Text)) {
            std::cerr << "Failed to open motion log CSV file for writing.\n";
            return img;
        }
    }
    QTextStream outStream(&csvFile);
    outStream << msecsSinceEpoch << "," << timeStr << "," << motionPercent << "\n";
    csvFile.close();

    // Update history with smoothed value
    motionHistory.push_back(smoothedMotionLevel);
    while (static_cast<int>(motionHistory.size()) > kMotionHistorySize)
        motionHistory.pop_front();

    QImage out = (img.format() == QImage::Format_ARGB32 ||
                  img.format() == QImage::Format_ARGB32_Premultiplied)
                     ? img
                     : img.convertToFormat(QImage::Format_ARGB32);

    // Layout
    const int margin    = 16;
    const int padX      = 10;
    const int padY      = 8;
    const int barWidth  = 2;  // pixels per sample
    const int barAreaW  = kMotionHistorySize * barWidth;
    const int barAreaH  = 120;
    const int labelH    = 20;
    const int boxW      = barAreaW + padX * 2;
    const int boxH      = barAreaH + labelH + padY * 2;
    const int boxX      = margin;
    const int boxY      = out.height() - boxH - margin;

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Background
    p.fillRect(boxX, boxY, boxW, boxH, QColor(0, 0, 0, 180));

    // Title
    QFont font;
    font.setPointSize(9);
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(boxX + padX, boxY + padY, barAreaW, labelH,
               Qt::AlignLeft | Qt::AlignVCenter, "Motion Level");

    // Draw grid line at 50 %
    int gridY = boxY + padY + labelH + barAreaH / 2;
    p.setPen(QColor(255, 255, 255, 60));
    p.drawLine(boxX + padX, gridY, boxX + padX + barAreaW, gridY);

    // Draw each bar
    int numSamples = static_cast<int>(motionHistory.size());
    int startX     = boxX + padX + (kMotionHistorySize - numSamples) * barWidth;
    int barBaseY   = boxY + padY + labelH + barAreaH;

    for (int i = 0; i < numSamples; ++i) {
        double level = motionHistory[static_cast<size_t>(i)];
        int barH = static_cast<int>(level * barAreaH);
        // No artificial floor: show true zero as no bar
        if (barH < 1)
            continue;

        // Color: green → yellow → red
        QColor col;
        if (level < 0.25)
            col = QColor(50, 220, 50);
        else if (level < 0.5)
            col = QColor(220, 200, 30);
        else
            col = QColor(220, 50, 50);

        p.fillRect(startX + i * barWidth, barBaseY - barH, barWidth, barH, col);
    }

    // Current-value readout
    font.setPointSize(8);
    font.setBold(false);
    p.setFont(font);
    p.setPen(Qt::white);
    QString pct = QString("%1 %").arg(motionLevel * 100, 0, 'f', 2);
    p.drawText(boxX + padX + barAreaW - 36, boxY + padY,
               36, labelH, Qt::AlignRight | Qt::AlignVCenter, pct);

    p.end();
    return out;
}

