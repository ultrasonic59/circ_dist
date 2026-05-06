#include "calibrationdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <cmath>

CalibrationDialog::CalibrationDialog(const QVector<CircleInfo>& circles,
    int preselectedId, QWidget* parent)
    : QDialog(parent), m_circles(circles), m_selectedId(preselectedId)
{
    setWindowTitle("📐 Калибровка");
    setMinimumWidth(450);
    setupUI();
}

void CalibrationDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === Инфо ===
    QLabel* header = new QLabel(
        "Выберите эталонную окружность и укажите её реальный размер.\n"
        "Используется диаметр окружности (2×радиус).");
    header->setWordWrap(true);
    header->setStyleSheet("color: #555; padding: 5px;");
    mainLayout->addWidget(header);

    // === Выбор окружности ===
    QGroupBox* circleGroup = new QGroupBox("Эталонная окружность");
    QVBoxLayout* circleLayout = new QVBoxLayout(circleGroup);

    m_circleCombo = new QComboBox;
    int defaultIdx = -1;

    for (int i = 0; i < m_circles.size(); ++i) {
        const auto& c = m_circles[i];
        QString text = QString("#%1 | Центр: (%2, %3) | R = %4 px | circ = %5")
            .arg(c.id)
            .arg(c.center.x(), 0, 'f', 1)
            .arg(c.center.y(), 0, 'f', 1)
            .arg(c.radius, 0, 'f', 1)
            .arg(c.circularity, 0, 'f', 3);
        m_circleCombo->addItem(text, c.id);
        if (c.id == m_selectedId) defaultIdx = i;
    }

    if (defaultIdx >= 0) m_circleCombo->setCurrentIndex(defaultIdx);
    circleLayout->addWidget(m_circleCombo);

    // Инфо о диаметре
    m_infoLabel = new QLabel("Диаметр: — px");
    m_infoLabel->setStyleSheet("font-weight: bold; color: #2a6; padding: 5px;");
    circleLayout->addWidget(m_infoLabel);

    circleGroup->setLayout(circleLayout);
    mainLayout->addWidget(circleGroup);

    // === Размер ===
    QGroupBox* sizeGroup = new QGroupBox("Реальный размер");
    QFormLayout* sizeLayout = new QFormLayout(sizeGroup);

    m_realSizeSpin = new QDoubleSpinBox;
    m_realSizeSpin->setRange(0.001, 999999.0);
    m_realSizeSpin->setDecimals(3);
    m_realSizeSpin->setValue(10.0);
    m_realSizeSpin->setSuffix("");
    m_realSizeSpin->setSingleStep(0.5);

    m_unitCombo = new QComboBox;
    m_unitCombo->addItems({ "mm", "cm", "m", "inch", "px" });

    sizeLayout->addRow("Размер:", m_realSizeSpin);
    sizeLayout->addRow("Единица:", m_unitCombo);

    sizeGroup->setLayout(sizeLayout);
    mainLayout->addWidget(sizeGroup);

    // === Кнопки ===
    QHBoxLayout* btnLayout = new QHBoxLayout;
    m_btnOk = new QPushButton("✅ Применить");
    m_btnCancel = new QPushButton("❌ Отмена");
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnOk);
    btnLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(btnLayout);

    // === Сигналы ===
    connect(m_circleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &CalibrationDialog::onCircleChanged);
    connect(m_btnOk, &QPushButton::clicked, this, &CalibrationDialog::onValidate);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    // Инициализация
    onCircleChanged(m_circleCombo->currentIndex());
}

void CalibrationDialog::onCircleChanged(int index) {
    if (index < 0 || index >= m_circles.size()) {
        m_infoLabel->setText("Диаметр: — px");
        return;
    }

    int id = m_circleCombo->currentData().toInt();
    for (const auto& c : m_circles) {
        if (c.id == id) {
            double diameter = 2.0 * c.radius;
            m_infoLabel->setText(
                QString("Диаметр: %1 px (радиус: %2 px)")
                .arg(diameter, 0, 'f', 2)
                .arg(c.radius, 0, 'f', 2));
            m_selectedId = id;
            break;
        }
    }
}

void CalibrationDialog::onValidate() {
    if (m_selectedId < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите окружность для калибровки");
        return;
    }

    if (m_realSizeSpin->value() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Реальный размер должен быть больше 0");
        return;
    }

    accept();
}

int CalibrationDialog::selectedCircleId() const {
    return m_selectedId;
}

double CalibrationDialog::realSize() const {
    return m_realSizeSpin->value();
}

QString CalibrationDialog::unit() const {
    return m_unitCombo->currentText();
}
