#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <asmp/asmp.h>
#include <asmp/mptask.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>

#define NUM_CORES 5
#define SAMPLE_SIZE 256
#define NUM_FREQS 50
#define PI  3.14159265358979323846

typedef struct {
  float real;
  float imag;
} dft_result_t;

typedef struct {
  float signal[SAMPLE_SIZE];
  float cos_table[SAMPLE_SIZE];
  float sin_table[SAMPLE_SIZE];
  dft_result_t result;
} dft_shared_t;

static const float input_freqs[NUM_FREQS] = {
  1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
  6.0f, 7.0f, 8.0f, 9.0f, 10.0f,
  11.0f, 12.0f, 13.0f, 14.0f, 15.0f,
  16.0f, 17.0f, 18.0f, 19.0f, 20.0f,
  21.0f, 22.0f, 23.0f, 24.0f, 25.0f,
  26.0f, 27.0f, 28.0f, 29.0f, 30.0f,
  31.0f, 32.0f, 33.0f, 34.0f, 35.0f,
  36.0f, 37.0f, 38.0f, 39.0f, 40.0f,
  41.0f, 42.0f, 43.0f, 44.0f, 45.0f,
  46.0f, 47.0f, 48.0f, 49.0f, 50.0f
};

void generate_signal(float *signal) {
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    signal[i] = sinf(2.0f * PI * 25.0f * i / SAMPLE_SIZE);
  }
}

void prepare_tables(float freq, dft_shared_t *data) {
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    float angle = 2.0f * PI * freq * i / SAMPLE_SIZE;
    data->cos_table[i] = cosf(angle);
    data->sin_table[i] = sinf(angle);
  }
}

int main(void) {
  mptask_t task[NUM_CORES];
  mpshm_t shm[NUM_CORES];
  mpmq_t mq[NUM_CORES];
  dft_shared_t *shm_data[NUM_CORES];
  uint32_t recv;
  int ret;

  printf("=== DFT Multicore Supervisor Start ===\n");

  for (int i = 0; i < NUM_CORES; i++) {
    char path[64];
    snprintf(path, sizeof(path), "/mnt/spif/DFTmulticore_worker%d", i);

    mptask_init(&task[i], path);
    mptask_assign(&task[i]);

    mpshm_init(&shm[i], 0x4000 + i, sizeof(dft_shared_t));
    mptask_bindobj(&task[i], &shm[i]);
    shm_data[i] = mpshm_attach(&shm[i], 0);
    memset(shm_data[i], 0, sizeof(dft_shared_t));

    mpmq_init(&mq[i], 0x5000 + i, mptask_getcpuid(&task[i]));
    mptask_bindobj(&task[i], &mq[i]);

    mptask_exec(&task[i]);
  }

  int round = NUM_FREQS / NUM_CORES;

  for (int r = 0; r < round; r++) {
    for (int c = 0; c < NUM_CORES; c++) {
      int idx = r * NUM_CORES + c;
      float freq = input_freqs[idx];
      generate_signal(shm_data[c]->signal);
      prepare_tables(freq, shm_data[c]);

      // 渡すのは周波数（またはダミー）で良い
      mpmq_send(&mq[c], 1, (uintptr_t)0);
    }

    for (int c = 0; c < NUM_CORES; c++) {
      mpmq_receive(&mq[c], &recv);
      dft_result_t *res = &shm_data[c]->result;
      printf("Freq %5.1f Hz: Real = %10.6f, Imag = %10.6f, Amp = %10.6f\n",
             input_freqs[r * NUM_CORES + c],
             res->real, res->imag,
             sqrtf(res->real * res->real + res->imag * res->imag));
    }
  }

  for (int i = 0; i < NUM_CORES; i++) {
    int wret;
    mptask_destroy(&task[i], false, &wret);
    mpshm_detach(&shm[i]);
    mpshm_destroy(&shm[i]);
    mpmq_destroy(&mq[i]);
  }

  printf("=== DFT Multicore Supervisor Done ===\n");
  return 0;
}
