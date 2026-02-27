#include "opencvqtprocessor.hpp"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QTextStream>
#include <QDateTime>
#include <algorithm>
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
                                              int sensitivity)
{
    if (currentFrame.isNull() || previousFrame.isNull())
        return 0.0;

    // Convert to BGR then grayscale – fully local, never touches shared buffers
    auto toBGRMat = [](const QImage &img) -> cv::Mat {
        QImage bgr = (img.format() == QImage::Format_BGR888)
                         ? img
                         : img.convertToFormat(QImage::Format_BGR888);
        cv::Mat m(bgr.height(), bgr.width(), CV_8UC3,
                  const_cast<uchar *>(bgr.bits()),
                  static_cast<size_t>(bgr.bytesPerLine()));
        cv::Mat gray;
        cv::cvtColor(m, gray, cv::COLOR_BGR2GRAY);
        return gray.clone();
    };

    cv::Mat grayA = toBGRMat(currentFrame);
    cv::Mat grayB = toBGRMat(previousFrame);
    if (grayA.empty() || grayB.empty())
        return 0.0;

    cv::Mat diff;
    cv::absdiff(grayA, grayB, diff);

    const double maxDiff = static_cast<double>(qMax(1, sensitivity));
    const int W = diff.cols;
    const int H = diff.rows;

    // Compute per-cell motion levels (row-major)
    lastCellLevels.assign(kGridRows * kGridCols, 0.0);
    for (int row = 0; row < kGridRows; ++row) {
        for (int col = 0; col < kGridCols; ++col) {
            int x0 = (col * W) / kGridCols;
            int y0 = (row * H) / kGridRows;
            int x1 = ((col + 1) * W) / kGridCols;
            int y1 = ((row + 1) * H) / kGridRows;
            cv::Mat cellDiff = diff(cv::Rect(x0, y0, x1 - x0, y1 - y0));
            double cellMean  = cv::mean(cellDiff)[0]; // 0–255
            lastCellLevels[row * kGridCols + col] = std::min(cellMean / maxDiff, 1.0);
        }
    }

    // Per-cell EMA smoothing (same factor as aggregate) to suppress I-frame flicker
    constexpr double CELL_SMOOTH = 0.85;
    if (smoothedCellLevels.size() != lastCellLevels.size())
        smoothedCellLevels.assign(lastCellLevels.size(), 0.0);
    for (size_t i = 0; i < lastCellLevels.size(); ++i)
        smoothedCellLevels[i] = CELL_SMOOTH * smoothedCellLevels[i] +
                                 (1.0 - CELL_SMOOTH) * lastCellLevels[i];

    // Aggregate: 60 % max cell (single hot spot stays prominent) + 40 % overall mean
    double maxCell  = *std::max_element(lastCellLevels.begin(), lastCellLevels.end());
    double sumCells = 0.0;
    for (double v : lastCellLevels) sumCells += v;
    double meanCells = sumCells / static_cast<double>(lastCellLevels.size());
    return std::min(0.6 * maxCell + 0.4 * meanCells, 1.0);
}

QImage OpenCVQtProcessor::applyGridMotionOverlay(const QImage &drawTarget)
{
    // smoothedCellLevels is populated by computeMotionLevel (called first in videoworker)
    if (smoothedCellLevels.empty() || drawTarget.isNull())
        return drawTarget;

    QImage out = drawTarget.convertToFormat(QImage::Format_ARGB32);
    const int W = out.width();
    const int H = out.height();

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, false);

    for (int row = 0; row < kGridRows; ++row) {
        for (int col = 0; col < kGridCols; ++col) {
            double level = smoothedCellLevels[static_cast<size_t>(row * kGridCols + col)];
            if (level < 0.15)
                continue;

            int x0 = (col * W) / kGridCols;
            int y0 = (row * H) / kGridRows;
            int cw = ((col + 1) * W) / kGridCols - x0;
            int ch = ((row + 1) * H) / kGridRows - y0;

            // Fill: green → yellow → red with alpha proportional to level
            QColor fill;
            int alpha = static_cast<int>(level * 180);
            if (level < 0.4)
                fill = QColor(50, 220, 50, alpha);
            else if (level < 0.7)
                fill = QColor(220, 200, 50, alpha);
            else
                fill = QColor(220, 50, 50, alpha);

            p.fillRect(x0, y0, cw, ch, fill);

            // Border for clearly active cells
            if (level > 0.35) {
                p.setPen(QPen(fill.lighter(170), 2));
                p.drawRect(x0 + 1, y0 + 1, cw - 2, ch - 2);
            }
        }
    }
    p.end();
    return out;
}

QImage OpenCVQtProcessor::applyMotionGraphOverlay(const QImage &img, double motionLevel)
{
    if (img.isNull())
        return img;

    // --- Spike rejection ---
    rawHistory.push_back(motionLevel);
    while (static_cast<int>(rawHistory.size()) > kSpikeWindowSize)
        rawHistory.pop_front();

    double filteredLevel = motionLevel;
    if (static_cast<int>(rawHistory.size()) >= 5) {
        std::vector<double> sorted(rawHistory.begin(), rawHistory.end());
        std::sort(sorted.begin(), sorted.end());
        double median = sorted[sorted.size() / 2];
        if (median > 0.0 && motionLevel > kSpikeFactor * median)
            filteredLevel = median;
    }

    // Exponential smoothing
    constexpr double SMOOTHING_FACTOR = 0.85;
    smoothedMotionLevel = SMOOTHING_FACTOR * smoothedMotionLevel +
                          (1.0 - SMOOTHING_FACTOR) * filteredLevel;

    // CSV logging
    QDateTime now       = QDateTime::currentDateTime();
    qint64 msecEpoch    = now.toMSecsSinceEpoch();
    QString timeStr     = now.toString("yyyy-MM-dd HH:mm:ss");
    QString motionPct   = QString::number(smoothedMotionLevel * 100, 'f', 2);
    if (!csvFile.isOpen()) {
        if (!csvFile.open(QIODevice::Append | QIODevice::Text)) {
            std::cerr << "Failed to open motion log CSV file for writing.\n";
            return img;
        }
    }
    QTextStream csv(&csvFile);
    csv << msecEpoch << "," << timeStr << "," << motionPct << "\n";
    csvFile.close();

    // Store per-row averages (mean of kGridCols cells per row) in cellHistory
    std::vector<double> rowAvgs(static_cast<size_t>(kGridRows), 0.0);
    if (!lastCellLevels.empty()) {
        for (int r = 0; r < kGridRows; ++r) {
            double sum = 0.0;
            for (int c = 0; c < kGridCols; ++c)
                sum += lastCellLevels[static_cast<size_t>(r * kGridCols + c)];
            rowAvgs[static_cast<size_t>(r)] = sum / kGridCols;
        }
    }
    cellHistory.push_back(rowAvgs);
    while (static_cast<int>(cellHistory.size()) > kMotionHistorySize)
        cellHistory.pop_front();

    // Also keep scalar history (for scale reference / value readout)
    motionHistory.push_back(smoothedMotionLevel);
    while (static_cast<int>(motionHistory.size()) > kMotionHistorySize)
        motionHistory.pop_front();

    QImage out = (img.format() == QImage::Format_ARGB32 ||
                  img.format() == QImage::Format_ARGB32_Premultiplied)
                     ? img
                     : img.convertToFormat(QImage::Format_ARGB32);

    // Layout
    const int margin   = 16;
    const int padX     = 10;
    const int padY     = 8;
    const int barWidth = 2;
    const int barAreaW = kMotionHistorySize * barWidth;
    const int barAreaH = 120;
    const int labelH   = 20;
    const int legendW  = 70; // extra width on right for row legend
    const int boxW     = barAreaW + padX * 2 + legendW;
    const int boxH     = barAreaH + labelH + padY * 2;
    const int boxX     = margin;
    const int boxY     = out.height() - boxH - margin;

    // Row colours (top row → bottom row)
    static const QColor rowColors[4] = {
        QColor(70,  140, 255), // row 0: blue
        QColor(70,  220, 140), // row 1: teal
        QColor(255, 180,  60), // row 2: amber
        QColor(220,  70,  70), // row 3: red
    };

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

    // 50 % grid line
    int gridY    = boxY + padY + labelH + barAreaH / 2;
    p.setPen(QColor(255, 255, 255, 60));
    p.drawLine(boxX + padX, gridY, boxX + padX + barAreaW, gridY);

    // Draw stacked bars: each bar is split into kGridRows coloured segments
    int numSamples = static_cast<int>(cellHistory.size());
    int startX     = boxX + padX + (kMotionHistorySize - numSamples) * barWidth;
    int barBaseY   = boxY + padY + labelH + barAreaH;

    for (int i = 0; i < numSamples; ++i) {
        double totalLevel = motionHistory[static_cast<size_t>(i)];
        int    totalBarH  = static_cast<int>(totalLevel * barAreaH);
        if (totalBarH < 1)
            continue;

        const auto &rows = cellHistory[static_cast<size_t>(i)];

        // Sum of row averages (to normalise proportional heights)
        double rowSum = 0.0;
        for (double rv : rows) rowSum += rv;
        if (rowSum < 1e-9) rowSum = 1e-9;

        int barX   = startX + i * barWidth;
        int segTop = barBaseY - totalBarH; // top of the entire stacked bar

        // Draw each row segment from bottom (row 3) to top (row 0)
        int curY = barBaseY;
        for (int r = kGridRows - 1; r >= 0; --r) {
            double rowFrac = rows[static_cast<size_t>(r)] / rowSum;
            int    segH    = static_cast<int>(rowFrac * totalBarH);
            if (segH < 1) continue;
            int segY = curY - segH;
            if (segY < segTop) { segH -= (segTop - segY); segY = segTop; }
            if (segH < 1) continue;
            p.fillRect(barX, segY, barWidth, segH, rowColors[r]);
            curY = segY;
        }
    }

    // Row colour legend (right of bar area)
    int lx = boxX + padX + barAreaW + 6;
    font.setPointSize(7);
    font.setBold(false);
    p.setFont(font);
    int legendRowH = barAreaH / kGridRows;
    for (int r = 0; r < kGridRows; ++r) {
        int ly = boxY + padY + labelH + r * legendRowH;
        p.fillRect(lx, ly + 2, 8, legendRowH - 4, rowColors[r]);
        p.setPen(Qt::white);
        p.drawText(lx + 11, ly, legendW - 12, legendRowH,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("R%1").arg(r));
    }

    // Percentage readout
    font.setPointSize(8);
    font.setBold(false);
    p.setFont(font);
    p.setPen(Qt::white);
    QString pct = QString("%1 %").arg(smoothedMotionLevel * 100, 0, 'f', 2);
    p.drawText(boxX + padX + barAreaW - 44, boxY + padY,
               44, labelH, Qt::AlignRight | Qt::AlignVCenter, pct);

    p.end();
    return out;
}

QImage OpenCVQtProcessor::applyBrightnessContrast(const QImage &image, int brightness, int contrast)
{
    if (image.isNull() || (brightness == 0 && contrast == 0))
        return image;

    cv::Mat mat = qImageToMat(image);
    if (mat.empty())
        return image;

    // Build a 1-channel 256-entry LUT; cv::LUT applies it uniformly to all 3 BGR channels.
    // This avoids format conversion, per-pixel Qt loops, and floating-point arithmetic at runtime.
    cv::Mat lut(1, 256, CV_8UC1);
    const double factor = (contrast + 100.0) / 100.0;
    uchar *p = lut.ptr<uchar>(0);
    for (int i = 0; i < 256; ++i) {
        double v = i + brightness;
        if (contrast != 0)
            v = 128.0 + (v - 128.0) * factor;
        p[i] = cv::saturate_cast<uchar>(v);
    }

    cv::LUT(mat, lut, workMat1);

    resultImage = QImage(workMat1.data, workMat1.cols, workMat1.rows,
                         workMat1.step, QImage::Format_BGR888).copy();
    return resultImage;
}

QImage OpenCVQtProcessor::applyColorTemperature(const QImage &image)
{
    if (image.isNull() || colorTemperature == 0)
        return image;

    cv::Mat mat = qImageToMat(image);
    if (mat.empty())
        return image;

    // Unified scale factors (both branches reduce to the same formula):
    //   warm (t<0): red boosted, blue cut
    //   cool (t>0): red cut, blue boosted
    const double t         = colorTemperature / 100.0; // [-1.0, 1.0]
    const double redScale  = 1.0 - t * 0.3;
    const double blueScale = 1.0 + t * 0.3;

    // Build a 3-channel LUT (256 entries, BGR order).
    // cv::LUT is SIMD-optimised – single integer pass, no float buffers, no channel split.
    cv::Mat lut(1, 256, CV_8UC3);
    for (int i = 0; i < 256; ++i) {
        lut.at<cv::Vec3b>(0, i)[0] = cv::saturate_cast<uchar>(i * blueScale); // B
        lut.at<cv::Vec3b>(0, i)[1] = static_cast<uchar>(i);                    // G unchanged
        lut.at<cv::Vec3b>(0, i)[2] = cv::saturate_cast<uchar>(i * redScale);  // R
    }

    cv::LUT(mat, lut, workMat1);

    resultImage = QImage(workMat1.data, workMat1.cols, workMat1.rows,
                         workMat1.step, QImage::Format_BGR888).copy();
    return resultImage;
}
