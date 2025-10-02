#ifndef FILTER2D_H
#define FILTER2D_H

#include <QImage>
#include <cstddef>

void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight);

void gaussianBlur(QImage &image, size_t size, double sigma);
double* createGaussianKernel1D(size_t size, double sigma);


double* createGaussianKernel(size_t size, double sigma);
double* createSharpenKernel();
double* createSobelXKernel();

#endif
