#ifndef CIRCLEDETECTOR_H
#define CIRCLEDETECTOR_H

#include <QVector>
#include <QPointF>
#include <QString>
#include <QPair>
#include <opencv2/opencv.hpp>

struct CircleInfo {
    QPointF center;      // центр окружности (px)
    double  radius;      // радиус (px)
    double  circularity; // ≈1 если идеальная окружность
    int     id;          // уникальный ID
};

struct CalibrationData {
    double pixelsPerUnit = 1.0;  // пикселей на мм/см
    QString unitType = "px";     // mm, cm, inch, px
    double referenceLength = 0;  // длина эталонного объекта в единицах
    bool isValid = false;
};

class CircleDetector {
public:
    CircleDetector();

    // Загрузка изображения
    bool loadImage(const QString& path);

    // Детекция окружностей
    QVector<CircleInfo> detectCircles(
        double dp = 1.2,
        double minDist = 60,
        double param1 = 100,
        double param2 = 50,
        int    minRad = 10,
        int    maxRad = 500
    );

    // Сравнение
    static bool areSimilar(const CircleInfo& a, const CircleInfo& b,
        double radiusTolerance = 0.2,
        double circularityTol = 0.3);

    // Расстояние (px)
    static double distancePx(const CircleInfo& a, const CircleInfo& b);

    // Расстояние (mm/cm/inch)
    double distanceReal(const CircleInfo& a, const CircleInfo& b) const;

    // Калибровка
    void setCalibration(const CalibrationData& calib);
    CalibrationData calibration() const;
    void calibrateFromCircle(const CircleInfo& circle, double realLength, const QString& unit);

    // Визуализация
    cv::Mat getDebugImage() const;
    cv::Mat getMarkedImage(const QVector<QPair<int, int>>& pairs,
        const QVector<double>& distances) const;
    cv::Mat getImageWithSelection(const QVector<int>& selectedIds) const;

    // Доступ к изображению
    cv::Mat getOriginalImage() const { return original; }

private:
    cv::Mat original;
    cv::Mat gray;
    QVector<CircleInfo> lastCircles;
    CalibrationData m_calibration;
    int nextId = 0;
};

#endif // CIRCLEDETECTOR_H
