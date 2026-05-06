#ifndef CIRCLEDETECTOR_H
#define CIRCLEDETECTOR_H

#include <QVector>
#include <QPointF>
#include <QString>
#include <opencv2/opencv.hpp>

struct CircleInfo {
    QPointF center;     // центр окружности
    double  radius;     // радиус
    double  circularity; // ≈1 если идеальная окружность
};

class CircleDetector {
public:
    CircleDetector();

    /// Загрузить изображение
    bool loadImage(const QString& path);

    /// Найти все окружности через HoughCircles
    QVector<CircleInfo> detectCircles(
        double dp       = 1.2,
        double minDist  = 60,
        double param1   = 100,
        double param2   = 50,
        int    minRad   = 10,
        int    maxRad   = 500
    );

    /// Сравнить две окружности (по радиусу и форме)
    static bool areSimilar(
        const CircleInfo& a,
        const CircleInfo& b,
        double radiusTolerance = 0.2,  // 20% разницы по радиусу
        double circularityTol  = 0.3
    );

    /// Евклидово расстояние между центрами
    static double distance(const CircleInfo& a, const CircleInfo& b);

    /// Отладочное изображение с обведёнными окружностями
    cv::Mat getDebugImage() const;

    /// Получить сырое изображение с линиями между выбранными парами
    cv::Mat getMarkedImage(const QVector<QPair<int,int>>& pairs) const;

private:
    cv::Mat original;
    cv::Mat gray;

    QVector<CircleInfo> lastCircles;
};

#endif // CIRCLEDETECTOR_H
