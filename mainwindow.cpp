#include "mainwindow.h"
#include "filter2d.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

const double MainWindow::SHARPEN_DEFAULTS[9] = {0.0, -1.5, 0.0, -1.5, 7.5, -1.5, 0.0, -1.5, 0.0};
const double MainWindow::SOBEL_DEFAULTS[9] = {-2.0, 0.0, 2.0, -4.0, 0.0, 4.0, -2.0, 0.0, 2.0};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    createTestImage();
}

void MainWindow::loadImage() {
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Открыть изображение", "",
                                                    "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!fileName.isEmpty()) {
        if (originalImage.load(fileName)) {
            processedImage = originalImage;
            updateDisplay();
            statusBar()->showMessage("Изображение " + fileName + " загружено.", 3000);
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось загрузить изображение.");
        }
    }
}

void MainWindow::saveImage() {
    if (processedImage.isNull()) {
        QMessageBox::warning(this, "Предупреждение", "Нет изображения для сохранения.");
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить изображение", "",
                                                    "PNG Image (*.png);;JPEG Image (*.jpg);;BMP Image (*.bmp)");
    if (!fileName.isEmpty()) {
        if (processedImage.save(fileName)) {
            statusBar()->showMessage("Изображение успешно сохранено в " + fileName, 3000);
        } else {
            QMessageBox::warning(this, "Ошибка сохранения", "Не удалось сохранить изображение.");
        }
    }
}

void MainWindow::applyFilter() {
    if (originalImage.isNull()) {
        QMessageBox::warning(this, "Предупреждение", "Сначала загрузите изображение!");
        return;
    }

    setControlsEnabled(false);
    statusBar()->showMessage("Применение фильтра...");

    QImage imageToProcess = originalImage.copy();
    int filterIndex = filterCombo->currentIndex();

    QFutureWatcher<QImage> *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher](){
        processedImage = watcher->result();
        updateDisplay();
        statusBar()->showMessage("Фильтр применен успешно!", 3000);
        setControlsEnabled(true);
        watcher->deleteLater();
    });

    if (filterIndex == 0) {
        int size = gaussSizeSpinBox->value();
        double sigma = gaussSigmaSpinBox->value();
        QFuture<QImage> future = QtConcurrent::run([=](){
            QImage resultImage = imageToProcess;
            gaussianBlur(resultImage, size, sigma);
            return resultImage;
        });
        watcher->setFuture(future);

    } else {
        double* kernelValues = new double[9];
        size_t kSize = 3;

        if (filterIndex == 1) {
            for(int i = 0; i < 9; ++i) kernelValues[i] = sharpenKernelInputs[i]->value();
        } else if (filterIndex == 2) {
            for(int i = 0; i < 9; ++i) kernelValues[i] = sobelKernelInputs[i]->value();
        }

        QFuture<QImage> future = QtConcurrent::run([=](){
            QImage resultImage = imageToProcess;
            filter2D(resultImage, kernelValues, kSize, kSize);
            delete[] kernelValues;
            return resultImage;
        });
        watcher->setFuture(future);
    }
}

void MainWindow::resetImage() {
    if (!originalImage.isNull()) {
        processedImage = originalImage.copy();
        updateDisplay();
        resetFilterParameters();
        statusBar()->showMessage("Изменения и параметры сброшены.", 2000);
    }
}

void MainWindow::onFilterChanged(int index) {
    parameterStack->setCurrentIndex(index);
}

void MainWindow::setControlsEnabled(bool enabled) {
    loadBtn->setEnabled(enabled);
    saveBtn->setEnabled(enabled);
    filterCombo->setEnabled(enabled);
    parameterStack->setEnabled(enabled);
    applyBtn->setEnabled(enabled);
    resetBtn->setEnabled(enabled);
}

void MainWindow::resetFilterParameters() {
    gaussSizeSpinBox->setValue(9);
    gaussSigmaSpinBox->setValue(4.0);
    for(int i = 0; i < 9; ++i) {
        sharpenKernelInputs[i]->setValue(SHARPEN_DEFAULTS[i]);
        sobelKernelInputs[i]->setValue(SOBEL_DEFAULTS[i]);
    }
}

QWidget* MainWindow::createKernelEditor(QDoubleSpinBox* inputs[9], const double defaultValues[9]) {
    QWidget *editorWidget = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(editorWidget);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            int index = i * 3 + j;
            inputs[index] = new QDoubleSpinBox();
            inputs[index]->setRange(-100.0, 100.0);
            inputs[index]->setDecimals(2);
            inputs[index]->setSingleStep(0.1);
            inputs[index]->setValue(defaultValues[index]);
            gridLayout->addWidget(inputs[index], i, j);
        }
    }
    return editorWidget;
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // === ПАНЕЛЬ ИЗОБРАЖЕНИЙ ===
    QWidget *imagePanel = new QWidget();
    QHBoxLayout *imageLayout = new QHBoxLayout(imagePanel);

    QVBoxLayout* originalVLayout = new QVBoxLayout();
    QLabel *originalTitle = new QLabel("Оригинал:");
    originalTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    originalLabel = new QLabel();
    originalLabel->setMinimumSize(300, 300);
    originalLabel->setAlignment(Qt::AlignCenter);
    originalLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 2px solid #ccc; }");
    originalVLayout->addWidget(originalTitle);
    originalVLayout->addWidget(originalLabel, 1);

    QVBoxLayout* processedVLayout = new QVBoxLayout();
    QLabel *processedTitle = new QLabel("После фильтрации:");
    processedTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    processedLabel = new QLabel();
    processedLabel->setMinimumSize(300, 300);
    processedLabel->setAlignment(Qt::AlignCenter);
    processedLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 2px solid #ccc; }");
    processedVLayout->addWidget(processedTitle);
    processedVLayout->addWidget(processedLabel, 1);

    imageLayout->addLayout(originalVLayout);
    imageLayout->addLayout(processedVLayout);

    // === ПАНЕЛЬ УПРАВЛЕНИЯ ===
    QWidget *controlPanel = new QWidget();
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlPanel->setMaximumWidth(400);

    loadBtn = new QPushButton("Загрузить изображение");
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadImage);

    saveBtn = new QPushButton("Сохранить результат");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveImage);

    filterCombo = new QComboBox();
    filterCombo->addItem("Размытие");
    filterCombo->addItem("Повышение резкости");
    filterCombo->addItem("Выделение краев");

    parameterStack = new QStackedWidget();

    // Страница Гауссово размытие
    QWidget *gaussPage = new QWidget();
    QFormLayout *gaussLayout = new QFormLayout(gaussPage);
    gaussSizeSpinBox = new QSpinBox();
    gaussSizeSpinBox->setRange(3, 99);
    gaussSizeSpinBox->setSingleStep(2);
    gaussSigmaSpinBox = new QDoubleSpinBox();
    gaussSigmaSpinBox->setRange(0.1, 50.0);
    gaussLayout->addRow("Размер ядра:", gaussSizeSpinBox);
    gaussLayout->addRow("Сигма:", gaussSigmaSpinBox);
    parameterStack->addWidget(gaussPage);

    parameterStack->addWidget(createKernelEditor(sharpenKernelInputs, SHARPEN_DEFAULTS));
    parameterStack->addWidget(createKernelEditor(sobelKernelInputs, SOBEL_DEFAULTS));

    resetFilterParameters();
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilterChanged);

    applyBtn = new QPushButton("Применить фильтр");
    applyBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::applyFilter);

    resetBtn = new QPushButton("Сбросить");
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetImage);

    infoWidget = new ImageInfoWidget();
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidget(infoWidget);
    scrollArea->setWidgetResizable(true);

    controlLayout->addWidget(loadBtn);
    controlLayout->addWidget(saveBtn);
    controlLayout->addSpacing(15);
    controlLayout->addWidget(new QLabel("Выберите фильтр:"));
    controlLayout->addWidget(filterCombo);
    controlLayout->addWidget(parameterStack);
    controlLayout->addSpacing(15);
    controlLayout->addWidget(applyBtn);
    controlLayout->addWidget(resetBtn);
    controlLayout->addSpacing(20);
    controlLayout->addWidget(scrollArea, 1);

    // Устанавливаем соотношение 3:1 (изображения : панель управления)
    mainLayout->addWidget(imagePanel, 3);
    mainLayout->addWidget(controlPanel, 1);

    setCentralWidget(centralWidget);
    setWindowTitle("Демонстрация фильтрации изображений");
    resize(1400, 800);
    statusBar()->showMessage("Готов к работе");
}

void MainWindow::createTestImage() {
    originalImage = QImage(400, 400, QImage::Format_RGB32);
    originalImage.fill(Qt::white);
    for (int y = 0; y < 400; ++y) {
        for (int x = 0; x < 400; ++x) {
            int dx1 = x - 100, dy1 = y - 100, dx2 = x - 300, dy2 = y - 300;
            if (dx1 * dx1 + dy1 * dy1 < 3600)
                originalImage.setPixel(x, y, qRgb(255, 100, 100));
            if (dx2 * dx2 + dy2 * dy2 < 3600)
                originalImage.setPixel(x, y, qRgb(100, 100, 255));
            if (x > 150 && x < 250 && y > 150 && y < 250) {
                int val = (x - 150) * 255 / 100;
                originalImage.setPixel(x, y, qRgb(val, 200, 255 - val));
            }
        }
    }
    processedImage = originalImage.copy();
    updateDisplay();
}

void MainWindow::updateDisplay() {
    originalLabel->setPixmap(QPixmap::fromImage(originalImage).scaled(
        originalLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    processedLabel->setPixmap(QPixmap::fromImage(processedImage).scaled(
        processedLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    infoWidget->setImage(processedImage);
}
