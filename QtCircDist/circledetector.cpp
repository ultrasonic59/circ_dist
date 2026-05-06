#include "circledetector.h"
#include <QDebug>
#include <cmath>

CircleDetector::CircleDetector() {}

bool CircleDetector::loadImage(const QString& path) {
    original = cv::imread(path.toStdString());
    if (original.empty()) {
        qWarning() << "Failed to load:" << path;
        return false;
    }
    cv::cvtColor(original, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(9, 9), 2);
    nextId = 0;
    m_calibration = CalibrationData();
    return true;
}

QVector<CircleInfo> CircleDetector::detectCircles(
    double dp, double minDist, double param1, double param2,
    int minRad, int maxRad)
{
    lastCircles.clear();
    std::vector<cv::Vec3f> circles;

    cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT,
        dp, minDist, param1, param2, minRad, maxRad);

    for (size_t i = 0; i < circles.size(); ++i) {
        CircleInfo info;
        info.center = QPointF(circles[i][0], circles[i][1]);
        info.radius = circles[i][2];
        info.id = nextId++;

        // Циркулярность
        cv::Mat mask = cv::Mat::zeros(gray.size(), CV_8UC1);
        cv::circle(mask,
            cv::Point(cvRound(circles[i][0]), cvRound(circles[i][1])),
            cvRound(circles[i][2]),
            cv::Scalar(255), cv::FILLED);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        if (!contours.empty()) {
            double area = cv::contourArea(contours[0]);
            double perim = cv::arcLength(contours[0], true);
            info.circularity = (perim > 0) ? (4.0 * CV_PI * area / (perim * perim)) : 0.0;
        }
        else {
            info.circularity = 0.0;
        }

        lastCircles.append(info);
    }

    return lastCircles;
}

bool CircleDetector::areSimilar(const CircleInfo& a, const CircleInfo& b,
    double radiusTolerance, double circularityTol)
{
    double rDiff = std::abs(a.radius - b.radius) / std::max(a.radius, b.radius);
    double cDiff = std::abs(a.circularity - b.circularity);
    return (rDiff <= radiusTolerance) && (cDiff <= circularityTol);
}

double CircleDetector::distancePx(const CircleInfo& a, const CircleInfo& b) {
    double dx = a.center.x() - b.center.x();
    double dy = a.center.y() - b.center.y();
    return std::sqrt(dx * dx + dy * dy);
}

double CircleDetector::distanceReal(const CircleInfo& a, const CircleInfo& b) const {
    double pxDist = distancePx(a, b);
    if (m_calibration.isValid && m_calibration.pixelsPerUnit > 0) {
        return pxDist / m_calibration.pixelsPerUnit;
    }
    return pxDist; // fallback
}

void CircleDetector::setCalibration(const CalibrationData& calib) {
    m_calibration = calib;
}

CalibrationData CircleDetector::calibration() const {
    return m_calibration;
}

void CircleDetector::calibrateFromCircle(const CircleInfo& circle,
    double realLength, const QString& unit)
{
    m_calibration.referenceLength = realLength;
    m_calibration.unitType = unit;
    // Используем диаметр (2*radius) как эталон
    double diameterPx = 2.0 * circle.radius;
    if (diameterPx > 0 && realLength > 0) {
        m_calibration.pixelsPerUnit = diameterPx / realLength;
        m_calibration.isValid = true;
    }
    else {
        m_calibration.isValid = false;
    }
}

cv::Mat CircleDetector::getDebugImage() const {
    if (original.empty()) return cv::Mat();
    cv::Mat result = original.clone();

    for (const auto& c : lastCircles) {
        cv::Point center(cvRound(c.center.x()), cvRound(c.center.y()));
        cv::circle(result, center, cvRound(c.radius), cv::Scalar(0, 255, 0), 2);
        cv::circle(result, center, 3, cv::Scalar(0, 0, 255), cv::FILLED);
        // ID
        cv::putText(result, std::to_string(c.id),
            cv::Point(center.x + 5, center.y - 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 2);
    }
    return result;
}

cv::Mat CircleDetector::getMarkedImage(const QVector<QPair<int, int>>& pairs,
    const QVector<double>& distances) const
{
    cv::Mat result = getDebugImage();

    for (int i = 0; i < pairs.size(); ++i) {
        int idx1 = pairs[i].first;
        int idx2 = pairs[i].second;
        if (idx1 < 0 || idx1 >= lastCircles.size() ||
            idx2 < 0 || idx2 >= lastCircles.size()) continue;

        cv::Point pt1(lastCircles[idx1].center.x(), lastCircles[idx1].center.y());
        cv::Point pt2(lastCircles[idx2].center.x(), lastCircles[idx2].center.y());

        cv::line(result, pt1, pt2, cv::Scalar(255, 0, 0), 2);

        // Текст расстояния
        QString distStr;
        if (m_calibration.isValid) {
            distStr = QString("%1 %2").arg(distances[i], 0, 'f', 2).arg(m_calibration.unitType);
        }
        else {
            distStr = QString("%1 px").arg(distances[i], 0, 'f', 1);
        }

        cv::Point mid((pt1.x + pt2.x) / 2, (pt1.y + pt2.y) / 2 - 10);
        cv::putText(result, distStr.toStdString(), mid,
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 0, 255), 2);
    }

    return result;
}

cv::Mat CircleDetector::getImageWithSelection(const QVector<int>& selectedIds) const {
    cv::Mat result = original.clone();

    for (const auto& c : lastCircles) {
        cv::Point center(cvRound(c.center.x()), cvRound(c.center.y()));
        bool selected = selectedIds.contains(c.id);

        cv::Scalar color = selected ? cv::Scalar(0, 255, 255) : cv::Scalar(0, 255, 0);
        int thickness = selected ? 3 : 1;
        cv::circle(result, center, cvRound(c.radius), color, thickness);
        cv::circle(result, center, 3, cv::Scalar(0, 0, 255), cv::FILLED);

        // ID
        cv::putText(result, std::to_string(c.id),
            cv::Point(center.x + 5, center.y - 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 2);
    }

    return result;
}
