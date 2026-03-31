#ifndef LIBCLOD_FFT_H
#define LIBCLOD_FFT_H

#include <clod/lib.h>
#include <stdint.h>

/// Input format.
#define CLOD_FFT_INPUT 0x0100
/// Output format.
#define CLOD_FFT_OUTPUT 0x010000

/// Time domain data in complex number format.
#define CLOD_FFT_TIME_COMPLEX 1
/// Time domain data in packed magnitude format. The second half of the array is ignored.
#define CLOD_FFT_TIME_MAG_PACKED 2
/// Frequency domain data in complex number format.
#define CLOD_FFT_FREQ_COMPLEX 3
/// Frequency domain data in magnitude&phase format.
#define CLOD_FFT_FREQ_MAG 4

/// Forward fourier transform.
#define CLOD_FFT_FORWARD     (CLOD_FFT_INPUT * CLOD_FFT_TIME_COMPLEX + CLOD_FFT_OUTPUT * CLOD_FFT_FREQ_COMPLEX)
/// Inverse fourier transform.
#define CLOD_FFT_INVERSE     (CLOD_FFT_INPUT * CLOD_FFT_FREQ_COMPLEX + CLOD_FFT_OUTPUT * CLOD_FFT_TIME_COMPLEX)
/// Forward fourier transform to magnitude&phase.
#define CLOD_FFT_EXTRACT     (CLOD_FFT_INPUT * CLOD_FFT_TIME_COMPLEX + CLOD_FFT_OUTPUT * CLOD_FFT_FREQ_MAG)
/// Inverse fourier transform to complex numbers.
#define CLOD_FFT_RECONSTRUCT (CLOD_FFT_INPUT * CLOD_FFT_FREQ_MAG     + CLOD_FFT_OUTPUT * CLOD_FFT_TIME_COMPLEX)

/**
 * Perform FFT operations on the provided array.
 * Length must be a power of two.
 * @param[in, out] data Array of floats in the specified format.
 * @param[in] len Size of \p data in elements. Must be a power of two.
 * An element is always a pair of floats. If the element format only has a single
 * @param[in] opt Options.
 */
CLOD_API CLOD_NONNULL(1)
void clod_fft(float *restrict data, size_t len, int opt);

#endif
