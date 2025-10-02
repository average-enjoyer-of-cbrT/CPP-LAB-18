#include "filter2d.h"
#include <QRgb>
#include <cmath>
#include <algorithm>

void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight) {
    if (image.isNull() || kernel == nullptr || kWidth == 0 || kHeight == 0) {
        return;
    }

    if (image.format() != QImage::Format_RGB32 &&
        image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_RGB32);
    }

    int width = image.width();
    int height = image.height();

    QImage original = image.copy();

    int kCenterX = static_cast<int>(kWidth) / 2;
    int kCenterY = static_cast<int>(kHeight) / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sumR = 0.0, sumG = 0.0, sumB = 0.0;

            for (size_t ky = 0; ky < kHeight; ++ky) {
                for (size_t kx = 0; kx < kWidth; ++kx) {
                    int pixelX = x + static_cast<int>(kx) - kCenterX;
                    int pixelY = y + static_cast<int>(ky) - kCenterY;

                    if (pixelX < 0) pixelX = -pixelX;
                    if (pixelX >= width) pixelX = 2 * width - pixelX - 2;
                    if (pixelY < 0) pixelY = -pixelY;
                    if (pixelY >= height) pixelY = 2 * height - pixelY - 2;

                    pixelX = std::max(0, std::min(width - 1, pixelX));
                    pixelY = std::max(0, std::min(height - 1, pixelY));

                    QRgb pixel = original.pixel(pixelX, pixelY);
                    double kernelValue = kernel[ky * kWidth + kx];

                    sumR += qRed(pixel) * kernelValue;
                    sumG += qGreen(pixel) * kernelValue;
                    sumB += qBlue(pixel) * kernelValue;
                }
            }

            int r = std::max(0, std::min(255, static_cast<int>(std::round(sumR))));
            int g = std::max(0, std::min(255, static_cast<int>(std::round(sumG))));
            int b = std::max(0, std::min(255, static_cast<int>(std::round(sumB))));

            image.setPixel(x, y, qRgb(r, g, b));
        }
    }
}

double* createGaussianKernel(size_t size, double sigma) {
    if (size % 2 == 0) {
        size++;
    }

    double *kernel = new double[size * size];
    double sum = 0.0;
    int center = static_cast<int>(size) / 2;

    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            int x = static_cast<int>(j) - center;
            int y = static_cast<int>(i) - center;
            double value = std::exp(-(x * x + y * y) / (2.0 * sigma * sigma));
            kernel[i * size + j] = value;
            sum += value;
        }
    }

    for (size_t i = 0; i < size * size; ++i) {
        kernel[i] /= sum;
    }

    return kernel;
}

double* createSharpenKernel() {
    double *kernel = new double[9];
    kernel[0] =  0.0; kernel[1] = -1.5; kernel[2] =  0.0;
    kernel[3] = -1.5; kernel[4] =  7.5; kernel[5] = -1.5;
    kernel[6] =  0.0; kernel[7] = -1.5; kernel[8] =  0.0;
    return kernel;
}

double* createSobelXKernel() {
    double *kernel = new double[9];
    kernel[0] = -2.0; kernel[1] = 0.0; kernel[2] = 2.0;
    kernel[3] = -4.0; kernel[4] = 0.0; kernel[5] = 4.0;
    kernel[6] = -2.0; kernel[7] = 0.0; kernel[8] = 2.0;
    return kernel;
}
