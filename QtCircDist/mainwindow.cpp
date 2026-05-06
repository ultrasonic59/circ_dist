#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Circle Distance Finder — OpenCV + Qt");
    resize(1100, 750);
    setupUI();
}

void MainWindow::setupUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // Кнопки
    QHBoxLayout* btnLayout = new QHBoxLayout;
    m_btnOpen    = new QPushButton("📂 Открыть");
    m_btnDetect  = new QPushButton("🔍 Найти окружности");
    m_btnSimilar = new QPushButton("📏 Похожие → расстояние");
    m_btnDetect->setEnabled(false);
    m_btnSimilar->setEnabled(false);

    btnLayout->addWidget(m_btnOpen);
    btnLayout->addWidget(m_btnDetect);
    btnLayout->addWidget(m_btnSimilar);
    mainLayout->addLayout(btnLayout);

    // Область изображения
    QScrollArea* scroll = new QScrollArea;
    m_imageLabel = new QLabel("Загрузите изображение");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    scroll->setWidget(m_imageLabel);
    scroll->setWidgetResizable(true);
    mainLayout->addWidget(scroll, 1);

    // Лог
    m_output = new QTextEdit;
    m_output->setReadOnly(true);
    m_output->setMaximumHeight(170);
    mainLayout->addWidget(m_output);

    // Сигналы
    connect(m_btnOpen,    &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(m_btnDetect,  &QPushButton::clicked, this, &MainWindow::onDetectCircles);
    connect(m_btnSimilar, &QPushButton::clicked, this, &MainWindow::onFindSimilar);
}

void MainWindow::onOpenImage() {
    QString path = QFileDialog::getOpenFileName(
        this, "Выберите изображение", "",
        "Images (*.png *.jpg *.jpeg *.bmp *.tif)");

    if (path.isEmpty()) return;

    if (detector.loadImage(path)) {
        cv::Mat img = cv::imread(path.toStdString());
        showImage(img);
        log("✅ Загружено: " + path);
        m_btnDetect->setEnabled(true);
        m_btnSimilar->setEnabled(false);
        circles.clear();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить изображение");
    }
}

void MainWindow::onDetectCircles() {
    circles = detector.detectCircles(1.2, 60, 100, 50, 10, 500);
    showImage(detector.getDebugImage());

    log(QString("🔵 Найдено окружностей: %1").arg(circles.size()));
    for (int i = 0; i < circles.size(); ++i) {
        const auto& c = circles[i];
        log(QString("  [%1] центр=(%2, %3) R=%4  circ=%5")
            .arg(i)
            .arg(c.center.x(), 0, 'f', 1)
            .arg(c.center.y(), 0, 'f', 1)
            .arg(c.radius, 0, 'f', 1)
            .arg(c.circularity, 0, 'f', 3));
    }

    m_btnSimilar->setEnabled(circles.size() >= 2);
}

void MainWindow::onFindSimilar() {
    if (circles.size() < 2) {
        log("⚠️ Меньше 2 окружностей!");
        return;
    }

    log("\n🔗 Поиск похожих окружностей...");
    QVector<QPair<int,int>> similarPairs;
    bool found = false;

    for (int i = 0; i < circles.size(); ++i) {
        for (int j = i + 1; j < circles.size(); ++j) {
            if (CircleDetector::areSimilar(circles[i], circles[j])) {
                double dist = CircleDetector::distance(circles[i], circles[j]);
                log(QString("✅ Похожи: [%1] ↔ [%2]  расстояние = %3 px")
                    .arg(i).arg(j).arg(dist, 0, 'f', 2));
                similarPairs.append({i, j});
                found = true;
            }
        }
    }

    if (!found) {
        log("❌ Похожих нет. Попробуйте изменить допуски в коде.");
    } else {
        showImage(detector.getMarkedImage(similarPairs));
    }
}

void MainWindow::showImage(const cv::Mat& mat) {
    if (mat.empty()) return;

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

    QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    m_imageLabel->setPixmap(QPixmap::fromImage(qimg)
                                .scaled(900, 700, Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
}

void MainWindow::log(const QString& msg) {
    m_output->append(msg);
}
