#include "mainwindow.h"
#include "calibrationdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QToolTip>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Circle Distance Finder — Qt + OpenCV");
    resize(1200, 800);
    setupUI();

    // Включаем отслеживание мыши
    m_imageLabel->setMouseTracking(true);
    setMouseTracking(true);
}

void MainWindow::setupUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // === Панель инструментов ===
    QHBoxLayout* toolLayout = new QHBoxLayout;

    m_btnOpen = new QPushButton("📂 Открыть");
    m_btnDetect = new QPushButton("🔍 Найти окружности");
    m_btnSimilar = new QPushButton("📏 Похожие → расстояние");
    m_btnCalibrate = new QPushButton("📐 Калибровка");
    m_btnClearSel = new QPushButton("✕ Сброс выбора");
    m_btnShowAll = new QPushButton("👁 Показать всё");

    m_btnDetect->setEnabled(false);
    m_btnSimilar->setEnabled(false);
    m_btnCalibrate->setEnabled(false);
    m_btnClearSel->setEnabled(false);
    m_btnShowAll->setEnabled(false);

    toolLayout->addWidget(m_btnOpen);
    toolLayout->addWidget(m_btnDetect);
    toolLayout->addWidget(m_btnSimilar);
    toolLayout->addWidget(m_btnCalibrate);
    toolLayout->addWidget(m_btnClearSel);
    toolLayout->addWidget(m_btnShowAll);
    mainLayout->addLayout(toolLayout);

    // === Подсказка ===
    QLabel* hint = new QLabel("💡 Кликните по окружности, чтобы выбрать. Shift+клик — добавить к выбору.");
    hint->setStyleSheet("color: #666; font-size: 11px; padding: 2px;");
    mainLayout->addWidget(hint);

    // === Изображение ===
    QScrollArea* scroll = new QScrollArea;
    m_imageLabel = new QLabel("Загрузите изображение");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: #1a1a2e; color: white;");
    scroll->setWidget(m_imageLabel);
    scroll->setWidgetResizable(true);
    mainLayout->addWidget(scroll, 1);

    // === Лог ===
    m_output = new QTextEdit;
    m_output->setReadOnly(true);
    m_output->setMaximumHeight(180);
    m_output->setStyleSheet("font-family: Consolas; font-size: 12px;");
    mainLayout->addWidget(m_output);

    // === Статус бар ===
    m_statusLabel = new QLabel("Готов");
    statusBar()->addWidget(m_statusLabel);

    // === Сигналы ===
    connect(m_btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(m_btnDetect, &QPushButton::clicked, this, &MainWindow::onDetectCircles);
    connect(m_btnSimilar, &QPushButton::clicked, this, &MainWindow::onFindSimilar);
    connect(m_btnCalibrate, &QPushButton::clicked, this, &MainWindow::onCalibrate);
    connect(m_btnClearSel, &QPushButton::clicked, this, &MainWindow::onClearSelection);
    connect(m_btnShowAll, &QPushButton::clicked, this, &MainWindow::onShowAll);
}

// ===================== МЫШЬ =====================

int MainWindow::findCircleAt(const QPoint& pos) {
    // Переводим координаты клика в координаты изображения
    double imgX = pos.x() / m_scaleX;
    double imgY = pos.y() / m_scaleY;

    double minDist = 15.0; // px допуск
    int found = -1;

    for (const auto& c : circles) {
        double dx = imgX - c.center.x();
        double dy = imgY - c.center.y();
        double dist = std::sqrt(dx * dx + dy * dy);

        // Проверяем: попадание внутрь окружности или близко к центру
        if (dist <= c.radius + 5) {
            if (found == -1 || dist < minDist) {
                minDist = dist;
                found = c.id;
            }
        }
    }
    return found;
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (circles.isEmpty() || !m_imageLabel->pixmap()) {
        QMainWindow::mousePressEvent(event);
        return;
    }

    QPoint labelPos = m_imageLabel->mapFrom(this, event->pos());
    if (labelPos.x() < 0 || labelPos.y() < 0 ||
        labelPos.x() > m_imageLabel->pixmap()->width() ||
        labelPos.y() > m_imageLabel->pixmap()->height()) {
        QMainWindow::mousePressEvent(event);
        return;
    }

    int id = findCircleAt(labelPos);
    if (id >= 0) {
        if (event->modifiers() & Qt::ShiftModifier) {
            // Shift+клик — добавляем/убираем
            if (selectedIds.contains(id))
                selectedIds.remove(id);
            else
                selectedIds.insert(id);
        }
        else {
            // Обычный клик — только эта
            selectedIds.clear();
            selectedIds.insert(id);
        }
        updateSelectionDisplay();
        m_btnClearSel->setEnabled(!selectedIds.isEmpty());

        // Показываем информацию
        for (const auto& c : circles) {
            if (c.id == id) {
                CalibrationData cal = detector.calibration();
                QString info;
                if (cal.isValid) {
                    double realR = c.radius / cal.pixelsPerUnit;
                    info = QString("Окружность #%1 | Центр: (%2, %3) | "
                        "Радиус: %4 px (%5 %6)")
                        .arg(id)
                        .arg(c.center.x(), 0, 'f', 1)
                        .arg(c.center.y(), 0, 'f', 1)
                        .arg(c.radius, 0, 'f', 1)
                        .arg(realR, 0, 'f', 2)
                        .arg(cal.unitType);
                }
                else {
                    info = QString("Окружность #%1 | Центр: (%2, %3) | "
                        "Радиус: %4 px | Циркулярность: %5")
                        .arg(id)
                        .arg(c.center.x(), 0, 'f', 1)
                        .arg(c.center.y(), 0, 'f', 1)
                        .arg(c.radius, 0, 'f', 1)
                        .arg(c.circularity, 0, 'f', 3);
                }
                m_statusLabel->setText(info);
                log("🔵 Выбрана: " + info);
                break;
            }
        }
    }
    else {
        // Клик мимо — сбрасываем
        if (!(event->modifiers() & Qt::ShiftModifier)) {
            selectedIds.clear();
            updateSelectionDisplay();
            m_btnClearSel->setEnabled(false);
            m_statusLabel->setText("Выбор сброшен");
        }
    }

    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    if (circles.isEmpty() || !m_imageLabel->pixmap()) {
        QMainWindow::mouseMoveEvent(event);
        return;
    }

    QPoint labelPos = m_imageLabel->mapFrom(this, event->pos());
    int id = findCircleAt(labelPos);

    if (id >= 0 && id != hoveredId) {
        hoveredId = id;
        for (const auto& c : circles) {
            if (c.id == id) {
                QString tip = QString("Окружность #%1\nЦентр: (%2, %3)\nR = %4 px")
                    .arg(id)
                    .arg(c.center.x(), 0, 'f', 1)
                    .arg(c.center.y(), 0, 'f', 1)
                    .arg(c.radius, 0, 'f', 1);
                m_imageLabel->setToolTip(tip);
                break;
            }
        }
        setCursor(Qt::PointingHandCursor);
    }
    else if (id < 0 && hoveredId >= 0) {
        hoveredId = -1;
        m_imageLabel->setToolTip("");
        setCursor(Qt::ArrowCursor);
    }

    QMainWindow::mouseMoveEvent(event);
}

// ===================== СЛОТЫ =====================

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
        m_btnCalibrate->setEnabled(false);
        m_btnClearSel->setEnabled(false);
        m_btnShowAll->setEnabled(false);
        circles.clear();
        selectedIds.clear();
        hoveredId = -1;
        m_statusLabel->setText("Изображение загружено");
    }
    else {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить изображение");
    }
}

void MainWindow::onDetectCircles() {
    circles = detector.detectCircles(1.2, 60, 100, 50, 10, 500);
    selectedIds.clear();
    showImage(detector.getDebugImage());

    log(QString("🔵 Найдено окружностей: %1").arg(circles.size()));
    for (const auto& c : circles) {
        log(QString("  [#%1] центр=(%2, %3) R=%4  circ=%5")
            .arg(c.id)
            .arg(c.center.x(), 0, 'f', 1)
            .arg(c.center.y(), 0, 'f', 1)
            .arg(c.radius, 0, 'f', 1)
            .arg(c.circularity, 0, 'f', 3));
    }

    m_btnSimilar->setEnabled(circles.size() >= 2);
    m_btnCalibrate->setEnabled(true);
    m_btnShowAll->setEnabled(true);
    m_statusLabel->setText(QString("Найдено %1 окружностей").arg(circles.size()));
}

void MainWindow::onFindSimilar() {
    if (circles.size() < 2) {
        log("⚠️ Меньше 2 окружностей!");
        return;
    }

    log("\n🔗 Поиск похожих окружностей...");
    QVector<QPair<int, int>> similarPairs;
    QVector<double> distances;
    bool found = false;

    // Если выбраны конкретные — сравниваем только их
    QVector<int> idsToCheck;
    if (!selectedIds.isEmpty()) {
        idsToCheck = selectedIds.values().toVector();
    }
    else {
        for (const auto& c : circles)
            idsToCheck.append(c.id);
    }

    for (int i = 0; i < idsToCheck.size(); ++i) {
        for (int j = i + 1; j < idsToCheck.size(); ++j) {
            const CircleInfo* a = nullptr;
            const CircleInfo* b = nullptr;

            for (const auto& c : circles) {
                if (c.id == idsToCheck[i]) a = &c;
                if (c.id == idsToCheck[j]) b = &c;
            }

            if (!a || !b) continue;

            if (CircleDetector::areSimilar(*a, *b)) {
                double dist;

                if (detector.calibration().isValid) {
                    dist = detector.distanceReal(*a, *b);
                    log(QString("✅ [%1] ↔ [%2]  расстояние = %3 %4")
                        .arg(a->id).arg(b->id)
                        .arg(dist, 0, 'f', 2)
                        .arg(detector.calibration().unitType));
                }
                else {
                    dist = CircleDetector::distancePx(*a, *b);
                    log(QString("✅ [%1] ↔ [%2]  расстояние = %3 px")
                        .arg(a->id).arg(b->id)
                        .arg(dist, 0, 'f', 2));
                }

                // Находим индексы
                int idxA = -1, idxB = -1;
                for (int k = 0; k < circles.size(); ++k) {
                    if (circles[k].id == a->id) idxA = k;
                    if (circles[k].id == b->id) idxB = k;
                }

                similarPairs.append({ idxA, idxB });
                distances.append(dist);
                found = true;
            }
        }
    }

    if (!found) {
        log("❌ Похожих не найдено. Попробуйте изменить допуски.");
        m_statusLabel->setText("Похожих окружностей не найдено");
    }
    else {
        showImage(detector.getMarkedImage(similarPairs, distances));
        m_statusLabel->setText(
            QString("Найдено %1 пар похожих окружностей").arg(similarPairs.size()));
    }
}

void MainWindow::onCalibrate() {
    if (circles.isEmpty()) {
        QMessageBox::information(this, "Калибровка",
            "Сначала найдите окружности на изображении.");
        return;
    }

    // Если выбрана одна окружность — используем её
    int calibId = -1;
    if (selectedIds.size() == 1) {
        calibId = *selectedIds.begin();
    }

    CalibrationDialog dlg(circles, calibId, this);
    if (dlg.exec() == QDialog::Accepted) {
        int circleId = dlg.selectedCircleId();
        double realSize = dlg.realSize();
        QString unit = dlg.unit();

        const CircleInfo* calibCircle = nullptr;
        for (const auto& c : circles) {
            if (c.id == circleId) {
                calibCircle = &c;
                break;
            }
        }

        if (calibCircle && realSize > 0) {
            detector.calibrateFromCircle(*calibCircle, realSize, unit);
            CalibrationData cal = detector.calibration();

            log(QString("📐 Калибровка выполнена!"));
            log(QString("   Эталон: окружность #%1 (диаметр %2 px)")
                .arg(circleId)
                .arg(2.0 * calibCircle->radius, 0, 'f', 1));
            log(QString("   Реальный размер: %1 %2")
                .arg(realSize, 0, 'f', 2).arg(unit));
            log(QString("   Масштаб: %1 px/%2")
                .arg(cal.pixelsPerUnit, 0, 'f', 4).arg(unit));

            m_statusLabel->setText(
                QString("Калибровка: %1 px/%2").arg(cal.pixelsPerUnit, 0, 'f', 4).arg(unit));

            updateSelectionDisplay();
        }
    }
}

void MainWindow::onClearSelection() {
    selectedIds.clear();
    updateSelectionDisplay();
    m_btnClearSel->setEnabled(false);
    m_statusLabel->setText("Выбор сброшен");
}

void MainWindow::onShowAll() {
    selectedIds.clear();
    showImage(detector.getDebugImage());
    m_statusLabel->setText("Показаны все окружности");
}

void MainWindow::updateSelectionDisplay() {
    if (selectedIds.isEmpty()) {
        showImage(detector.getDebugImage());
        return;
    }

    QVector<int> ids = selectedIds.values().toVector();
    showImage(detector.getImageWithSelection(ids));

    // Если 2 выбраны — сразу показываем расстояние между ними
    if (ids.size() == 2 && detector.calibration().isValid) {
        const CircleInfo* a = nullptr;
        const CircleInfo* b = nullptr;
        for (const auto& c : circles) {
            if (c.id == ids[0]) a = &c;
            if (c.id == ids[1]) b = &c;
        }
        if (a && b) {
            double dist = detector.distanceReal(*a, *b);
            QString msg = QString("📏 Расстояние между #%1 и #%2: %3 %4")
                .arg(a->id).arg(b->id)
                .arg(dist, 0, 'f', 2)
                .arg(detector.calibration().unitType);
            log(msg);
            m_statusLabel->setText(msg);
        }
    }
}

void MainWindow::showImage(const cv::Mat& mat) {
    if (mat.empty()) return;

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

    QImage qimg(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);

    // Масштабируем с сохранением пропорций
    int maxW = m_imageLabel->width() - 20;
    int maxH = m_imageLabel->height() - 20;
    if (maxW < 100) maxW = 900;
    if (maxH < 100) maxH = 700;

    QPixmap pix = QPixmap::fromImage(qimg);
    QPixmap scaled = pix.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_scaleX = (double)scaled.width() / qimg.width();
    m_scaleY = (double)scaled.height() / qimg.height();

    m_imageLabel->setPixmap(scaled);
}

void MainWindow::log(const QString& msg) {
    m_output->append(msg);
}
