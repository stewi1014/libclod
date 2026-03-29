#ifndef LIBCLOD_FFT_H
#define LIBCLOD_FFT_H

#include <clod/lib.h>

/**
 * Perform a fast-fourier transform on the provided array of complex numbers.
 * Length must be a power of two.
 * @param[in, out] data Array of complex numbers to perform FFT on.
 * Each number has two floats for real and imaginary components.
 * @param[in] len Size of \p data in numbers (2 floats).
 * Must be a power of two.
 * @param[in] invert True makes the method perform an inverse FFT.
 */
CLOD_API CLOD_NONNULL(1)
void clod_fft(float *restrict data, size_t len, bool invert);

#endif
