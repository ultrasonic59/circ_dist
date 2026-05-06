#include "circledetector.h"
#include <QDebug>
#include <cmath>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

CircleDetector::CircleDetector() {}

bool CircleDetector::loadImage(const QString& path) {
    original = cv::imread(path.toStdString());
    if (original.empty()) {
        qWarning() << "Failed to load:" << path;
        return false;
    }
    cv::cvtColor(original, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(9, 9), 2); // сглаживание
    return true;
}

QVector<CircleInfo> CircleDetector::detectCircles(
    double dp, double minDist, double param1, double param2,
    int minRad, int maxRad)
{
    lastCircles.clear();
    std::vector<cv::Vec3f> circles;

    cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT,
                     dp, minDist, param1, param2,
                     minRad, maxRad);

    for (size_t i = 0; i < circles.size(); ++i) {
        CircleInfo info;
        info.center = QPointF(circles[i][0], circles[i][1]);
        info.radius = circles[i][2];

        // Вычисление циркулярности через контур
        cv::Mat mask = cv::Mat::zeros(gray.size(), CV_8UC1);
        cv::circle(mask,
                   cv::Point(cvRound(circles[i][0]), cvRound(circles[i][1])),
                   cvRound(circles[i][2]),
                   cv::Scalar(255), cv::FILLED);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        if (!contours.empty()) {
            double area  = cv::contourArea(contours[0]);
            double perim = cv::arcLength(contours[0], true);
            if (perim > 0) {
                info.circularity = 4.0 * CV_PI * area / (perim * perim);
            } else {
                info.circularity = 0.0;
            }
        } else {
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

double CircleDetector::distance(const CircleInfo& a, const CircleInfo& b) {
    double dx = a.center.x() - b.center.x();
    double dy = a.center.y() - b.center.y();
    return std::sqrt(dx * dx + dy * dy);
}

cv::Mat CircleDetector::getDebugImage() const {
    if (original.empty()) return cv::Mat();
    cv::Mat result = original.clone();

    for (const auto& c : lastCircles) {
        cv::Point center(cvRound(c.center.x()), cvRound(c.center.y()));
        cv::circle(result, center, cvRound(c.radius), cv::Scalar(0, 255, 0), 2);
        cv::circle(result, center, 3, cv::Scalar(0, 0, 255), cv::FILLED);
    }
    return result;
}

cv::Mat CircleDetector::getMarkedImage(const QVector<QPair<int,int>>& pairs) const {
    cv::Mat result = getDebugImage();

    for (const auto& p : pairs) {
        if (p.first < 0 || p.first >= lastCircles.size() ||
            p.second < 0 || p.second >= lastCircles.size()) continue;

        cv::Point pt1(lastCircles[p.first].center.x(),
                      lastCircles[p.first].center.y());
        cv::Point pt2(lastCircles[p.second].center.x(),
                      lastCircles[p.second].center.y());

        cv::line(result, pt1, pt2, cv::Scalar(255, 0, 0), 2);
    }
    return result;
}
