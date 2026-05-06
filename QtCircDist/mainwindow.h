#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include "circledetector.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onOpenImage();
    void onDetectCircles();
    void onFindSimilar();

private:
    void setupUI();
    void showImage(const cv::Mat& mat);
    void log(const QString& msg);

    CircleDetector   detector;
    QVector<CircleInfo> circles;

    QLabel*       m_imageLabel;
    QPushButton*  m_btnOpen;
    QPushButton*  m_btnDetect;
    QPushButton*  m_btnSimilar;
    QTextEdit*    m_output;
};

#endif // MAINWINDOW_H
