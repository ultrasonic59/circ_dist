#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QStatusBar>
#include <QMouseEvent>
#include <QSet>
#include "circledetector.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void onOpenImage();
    void onDetectCircles();
    void onFindSimilar();
    void onCalibrate();
    void onClearSelection();
    void onShowAll();

private:
    void setupUI();
    void showImage(const cv::Mat& mat);
    void log(const QString& msg);
    int findCircleAt(const QPoint& pos);
    void updateSelectionDisplay();

    CircleDetector   detector;
    QVector<CircleInfo> circles;
    QSet<int> selectedIds;       // выбранные ID
    int hoveredId = -1;          // ID под курсором

    // UI
    QLabel* m_imageLabel;
    QPushButton* m_btnOpen;
    QPushButton* m_btnDetect;
    QPushButton* m_btnSimilar;
    QPushButton* m_btnCalibrate;
    QPushButton* m_btnClearSel;
    QPushButton* m_btnShowAll;
    QTextEdit* m_output;
    QLabel* m_statusLabel;

    double m_scaleX = 1.0;
    double m_scaleY = 1.0;
};

#endif // MAINWINDOW_H
