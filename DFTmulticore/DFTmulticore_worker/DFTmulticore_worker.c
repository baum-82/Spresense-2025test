#include <asmp/types.h>
#include <asmp/mpshm.h>
#include <asmp/mpmq.h>
#include <asmp/asmp.h>
#include <nuttx/arch.h>
#include <math.h>

#define SAMPLE_SIZE 256

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

void perform_dft(dft_shared_t *data) {
  float real = 0.0f;
  float imag = 0.0f;

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    real += data->signal[i] * data->cos_table[i];
    imag -= data->signal[i] * data->sin_table[i];
  }

  data->result.real = real;
  data->result.imag = imag;
}

int main(void) {
  mpshm_t shm;
  mpmq_t mq;
  dft_shared_t *data;
  uint32_t recv;

  mpshm_init(&shm, 0x4000 + up_cpu_index(), sizeof(dft_shared_t));
  data = (dft_shared_t *)mpshm_attach(&shm, 0);

  mpmq_init(&mq, 0x5000 + up_cpu_index(), 0);

  while (1) {
    mpmq_receive(&mq, &recv); // 開始通知

    perform_dft(data);

    mpmq_send(&mq, 1, 0); // 完了通知
  }

  return 0;
}
