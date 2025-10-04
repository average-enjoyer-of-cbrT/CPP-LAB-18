#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QPushButton>
#include "imageinfowidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void loadImage();
    void saveImage();
    void applyFilter();
    void resetImage();
    void onFilterChanged(int index);

private:
    void setControlsEnabled(bool enabled);
    void resetFilterParameters();
    QWidget* createKernelEditor(QDoubleSpinBox* inputs[9], const double defaultValues[9]);
    void setupUI();
    void createTestImage();
    void updateDisplay();

    static const double SHARPEN_DEFAULTS[9];
    static const double SOBEL_DEFAULTS[9];

    QImage originalImage, processedImage;
    QLabel *originalLabel, *processedLabel;
    ImageInfoWidget *infoWidget;
    QPushButton *loadBtn, *saveBtn, *applyBtn, *resetBtn;
    QComboBox *filterCombo;
    QStackedWidget *parameterStack;
    QSpinBox *gaussSizeSpinBox;
    QDoubleSpinBox *gaussSigmaSpinBox;
    QDoubleSpinBox *sharpenKernelInputs[9];
    QDoubleSpinBox *sobelKernelInputs[9];
};

#endif // MAINWINDOW_H
