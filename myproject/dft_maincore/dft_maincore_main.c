/* spresense_maincore_dft.c */
/* メインコア単独で 2.5Hz 間隔の周波数で DFT 計算を行う */

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define SAMPLE_SIZE 256
#define PI 3.14159265358979323846
#define NUM_FREQS 20

/* 計算対象周波数: 0, 2.5, 5, 7.5, 10, 12.5, 15, 17.5, 20, 22.5 Hz */
static const float freq_list[NUM_FREQS] = {
  0.0f, 2.5f, 5.0f, 7.5f, 10.0f,
  12.5f, 15.0f, 17.5f, 20.0f, 22.5f,
  25.0f, 27.5f, 30.0f, 32.5f, 35.0f,
  37.5f, 40.0f, 42.5f, 45.0f, 47.5f
};

float input_signal[SAMPLE_SIZE];

static void generate_signal();
static void dft(float freq, float *out_real, float *out_imag);

int main(int argc, char *argv[]) {
  int i;
  float real, imag, spec;
  struct timeval start, end;
  long elapsed_ms;

  printf("Spresense main core only DFT calculation\n");

  generate_signal();

  gettimeofday(&start, NULL);

  for (i = 0; i < NUM_FREQS; i++) {
    dft(freq_list[i], &real, &imag);
    spec = sqrt(pow(real, 2) + pow(imag, 2));
    printf("Freq %5.1f Hz: Real = %10.6f, Imag = %10.6f, Amp_spectrum = %10.6f\n", freq_list[i], real, imag, spec);
  }

  gettimeofday(&end, NULL);

  elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 +
                    (end.tv_usec - start.tv_usec) / 1000;

  printf("\nTotal calculation time: %ld ms\n", elapsed_ms);

  return 0;
}


static void generate_signal() {
  int i;
  for (i = 0; i < SAMPLE_SIZE; i++) {
    input_signal[i] = sinf(2 * PI * 2 * i / SAMPLE_SIZE)
                      + 2*sinf(2 * PI * 5 * i / SAMPLE_SIZE);
  }
}

static void dft(float freq, float *out_real, float *out_imag) {
  float real = 0.0f;
  float imag = 0.0f;
  float angle;
  int i;

  for (i = 0; i < SAMPLE_SIZE; i++) {
    angle = 2 * PI * freq * i / SAMPLE_SIZE;
    real += input_signal[i] * cosf(angle);
    imag -= input_signal[i] * sinf(angle);
  }

  *out_real = real;
  *out_imag = imag;
}