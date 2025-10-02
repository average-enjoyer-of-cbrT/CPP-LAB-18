// --- START OF FILE main.cpp ---

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QGroupBox>
#include <QStatusBar>
#include <QDebug>
#include "filter2d.h"
#include "imageinfowidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        createTestImage();
    }

private slots:
    void loadImage() {
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

    void saveImage() {
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

    void applyFilter() {
        if (originalImage.isNull()) {
            QMessageBox::warning(this, "Предупреждение",
                                 "Сначала загрузите изображение!");
            return;
        }

        processedImage = originalImage.copy();

        int filterIndex = filterCombo->currentIndex();
        double *kernel = nullptr;
        size_t kSize = 3;

        try {
            if (filterIndex == 0) { // Размытие
                int size = gaussSizeSpinBox->value();
                if (size % 2 == 0) size++; // Размер должен быть нечетным
                double sigma = gaussSigmaSpinBox->value();
                kSize = static_cast<size_t>(size);
                kernel = createGaussianKernel(kSize, sigma);
            } else if (filterIndex == 1) { // Повышение резкости
                kSize = 3;
                kernel = new double[kSize * kSize];
                for(size_t i = 0; i < kSize * kSize; ++i) {
                    kernel[i] = sharpenKernelInputs[i]->value();
                }
            } else if (filterIndex == 2) { // Выделение краев
                kSize = 3;
                kernel = new double[kSize * kSize];
                for(size_t i = 0; i < kSize * kSize; ++i) {
                    kernel[i] = sobelKernelInputs[i]->value();
                }
            }
        } catch (const std::bad_alloc&) {
            QMessageBox::critical(this, "Ошибка", "Не удалось выделить память для ядра фильтра.");
            return;
        }


        if (kernel) {
            statusBar()->showMessage("Применение фильтра...");
            QApplication::processEvents(); // Обновить UI
            filter2D(processedImage, kernel, kSize, kSize);
            delete[] kernel;
            statusBar()->showMessage("Фильтр применен успешно!", 3000);
            updateDisplay();
        }
    }

    // ОБНОВЛЕННЫЙ СЛОТ
    void resetImage() {
        if (!originalImage.isNull()) {
            processedImage = originalImage.copy();
            updateDisplay();
            resetFilterParameters(); // <-- ДОБАВЛЕНО: сброс параметров в UI
            statusBar()->showMessage("Изменения и параметры сброшены.", 2000); // <-- ОБНОВЛЕНО сообщение
        }
    }

    void onFilterChanged(int index) {
        parameterStack->setCurrentIndex(index);
    }

private:
    // НОВЫЙ МЕТОД для сброса UI к значениям по умолчанию
    void resetFilterParameters() {
        // Сброс параметров размытия
        gaussSizeSpinBox->setValue(9);
        gaussSigmaSpinBox->setValue(4.0);

        // Сброс ядра повышения резкости
        for(int i = 0; i < 9; ++i) {
            sharpenKernelInputs[i]->setValue(SHARPEN_DEFAULTS[i]);
        }

        // Сброс ядра выделения краев
        for(int i = 0; i < 9; ++i) {
            sobelKernelInputs[i]->setValue(SOBEL_DEFAULTS[i]);
        }
    }

    // Вспомогательная функция для создания редактора ядра 3x3
    QWidget* createKernelEditor(QDoubleSpinBox* inputs[9], const double defaultValues[9]) {
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


    void setupUI() {
        QWidget *centralWidget = new QWidget(this);
        QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

        // --- Панель с изображениями ---
        QWidget *imagePanel = new QWidget();
        QHBoxLayout *imageLayout = new QHBoxLayout(imagePanel);

        // Оригинальное изображение
        QVBoxLayout* originalVLayout = new QVBoxLayout();
        QLabel *originalTitle = new QLabel("Оригинал:");
        originalTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
        originalLabel = new QLabel();
        originalLabel->setMinimumSize(300, 300);
        originalLabel->setAlignment(Qt::AlignCenter);
        originalLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 2px solid #ccc; }");
        originalVLayout->addWidget(originalTitle);
        originalVLayout->addWidget(originalLabel, 1);

        // Обработанное изображение
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


        // --- Панель управления ---
        QWidget *controlPanel = new QWidget();
        QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
        controlPanel->setMaximumWidth(400);

        QPushButton *loadBtn = new QPushButton("Загрузить изображение");
        connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadImage);

        QPushButton *saveBtn = new QPushButton("Сохранить результат");
        connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveImage);


        filterCombo = new QComboBox();
        filterCombo->addItem("Размытие");
        filterCombo->addItem("Повышение резкости");
        filterCombo->addItem("Выделение краев");

        // --- Создание виджетов с параметрами для каждого фильтра ---
        parameterStack = new QStackedWidget();

        // 1. Параметры для размытия
        QWidget *gaussPage = new QWidget();
        QFormLayout *gaussLayout = new QFormLayout(gaussPage);
        gaussSizeSpinBox = new QSpinBox();
        gaussSizeSpinBox->setRange(3, 99);
        gaussSizeSpinBox->setSingleStep(2);
        gaussSizeSpinBox->setValue(9);
        gaussSigmaSpinBox = new QDoubleSpinBox();
        gaussSigmaSpinBox->setRange(0.1, 50.0);
        gaussSigmaSpinBox->setValue(4.0);
        gaussLayout->addRow("Размер ядра:", gaussSizeSpinBox);
        gaussLayout->addRow("Сигма:", gaussSigmaSpinBox);
        parameterStack->addWidget(gaussPage);

        // 2. Параметры для повышения резкости
        parameterStack->addWidget(createKernelEditor(sharpenKernelInputs, SHARPEN_DEFAULTS));

        // 3. Параметры для выделения краев
        parameterStack->addWidget(createKernelEditor(sobelKernelInputs, SOBEL_DEFAULTS));

        connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);


        QPushButton *applyBtn = new QPushButton("Применить фильтр");
        applyBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
        connect(applyBtn, &QPushButton::clicked, this, &MainWindow::applyFilter);

        QPushButton *resetBtn = new QPushButton("Сбросить");
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

        mainLayout->addWidget(imagePanel, 1);
        mainLayout->addWidget(controlPanel);

        setCentralWidget(centralWidget);
        setWindowTitle("Демонстрация фильтрации изображений");
        resize(1200, 700);

        statusBar()->showMessage("Готов к работе");
    }

    void createTestImage() {
        originalImage = QImage(400, 400, QImage::Format_RGB32);
        originalImage.fill(Qt::white);

        for (int y = 0; y < 400; ++y) {
            for (int x = 0; x < 400; ++x) {
                int dx1 = x - 100;
                int dy1 = y - 100;
                int dx2 = x - 300;
                int dy2 = y - 300;

                if (dx1 * dx1 + dy1 * dy1 < 3600) {
                    originalImage.setPixel(x, y, qRgb(255, 100, 100));
                }
                if (dx2 * dx2 + dy2 * dy2 < 3600) {
                    originalImage.setPixel(x, y, qRgb(100, 100, 255));
                }

                if (x > 150 && x < 250 && y > 150 && y < 250) {
                    int val = (x - 150) * 255 / 100;
                    originalImage.setPixel(x, y, qRgb(val, 200, 255 - val));
                }
            }
        }

        processedImage = originalImage.copy();
        updateDisplay();
    }

    void updateDisplay() {
        // Отображение в QLabel
        originalLabel->setPixmap(QPixmap::fromImage(originalImage).scaled(
            originalLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        processedLabel->setPixmap(QPixmap::fromImage(processedImage).scaled(
            processedLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // Обновление информации
        infoWidget->setImage(processedImage);
    }

    // ДОБАВЛЕНО: константы для значений по умолчанию
    const double SHARPEN_DEFAULTS[9] = {0.0, -1.5, 0.0, -1.5, 7.5, -1.5, 0.0, -1.5, 0.0};
    const double SOBEL_DEFAULTS[9] = {-2.0, 0.0, 2.0, -4.0, 0.0, 4.0, -2.0, 0.0, 2.0};

    // Изображения
    QImage originalImage;
    QImage processedImage;

    // Элементы UI для отображения
    QLabel *originalLabel;
    QLabel *processedLabel;
    ImageInfoWidget *infoWidget;

    // Элементы UI для управления
    QComboBox *filterCombo;
    QPushButton *applyBtn;
    QPushButton *resetBtn;

    // Виджеты для параметров
    QStackedWidget *parameterStack;
    // - для размытия
    QSpinBox *gaussSizeSpinBox;
    QDoubleSpinBox *gaussSigmaSpinBox;
    // - для резкости
    QDoubleSpinBox *sharpenKernelInputs[9];
    // - для выделения краев
    QDoubleSpinBox *sobelKernelInputs[9];

};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"
