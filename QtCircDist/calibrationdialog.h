#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include "circledetector.h"

class CalibrationDialog : public QDialog {
    Q_OBJECT
public:
    CalibrationDialog(const QVector<CircleInfo>& circles,
                      int preselectedId = -1,
                      QWidget* parent = nullptr);

    int selectedCircleId() const;
    double realSize() const;
    QString unit() const;

private slots:
    void onCircleChanged(int index);
    void onValidate();

private:
    void setupUI();

    QComboBox*      m_circleCombo;
    QDoubleSpinBox* m_realSizeSpin;
    QComboBox*      m_unitCombo;
    QLabel*         m_infoLabel;
    QPushButton*    m_btnOk;
    QPushButton*    m_btnCancel;

    QVector<CircleInfo> m_circles;
    int m_selectedId = -1;
};

#endif // CALIBRATIONDIALOG_H
