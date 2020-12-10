/*
 * noise_management.c
 *
 *  Created on: 2020年12月10日
 *      Author: 123
 */




#include "noise_management.h"

#include "array_utilities.h"
#include "arm_math.h"
#include "filter_coef.h"

#define NTYPE 3
#define STYPE 1
#define SPRAM 0.4
#define NUM_BANDS 6

typedef struct
{
  float* taps;
  size_t tap_size;
} fir_formii;

typedef struct
{
  float prob;
  float peak_init;
  float npow_init;
  float valley_init;
} noise_management;


float** current_delay_block;
float** next_delay_block;
noise_management **nm;

float *in_float;
float *out_float;
float **out_filter;
float **outSE;
float *outSE_sum;

/* Attack time in msec */
float att;
/* Attack filter coefficient */
float aLP;
/* Release time in msec */
float rel;
/* Release filter coefficient */
float rLP;
/* Parameters for the valley detection */
/* Attack filter coefficient */
float aLPv;
/* Release filter coefficient */
float rLPv;
/* Parameters and storage for the Arslan et al. power averaging */
/* Maximum increase in dB/sec */
float inc;
/* Maximum decrease in dB/sec */
float dec;
float lin_inc;
float lin_dec;
float a1;
float a2;
/* Parameters for the Hirsch and Ehrlicher weighted noise averaging */
float tau;
/* LP filter coefficient */
float aHE;
/* Speech/noise threshold */
float bHE;
/* Parameters for the Cohen and Berdugo MCRA */
float delta; //floathreshold for P[speech]
float aCB; //LP filter coefficient for ave prob
float bCB; //LP filter coefficient for noise ave



void filter_process(float *in, float **out, size_t window_size) {
  // init filters
  fir_formii filters[NUM_BANDS];
  for (int band = 0; band < NUM_BANDS; band++) {
    filters[band].taps = subband_filter[band];
    filters[band].tap_size = BAND_FILT_LEN;
  }
  // process
  for(int k = 0; k < SIX_BANDS; k++) {
    float* temp;

    float* taps = filters[k].taps;
    size_t tap_length = filters[k].tap_size;

    for (size_t i = 0; i < window_size; i++) {
      for (size_t j = 0; j < tap_length - 1; j++) {
        next_delay_block[k][j] = current_delay_block[k][j+1] + taps[j] * in[i];
      }
      next_delay_block[k][tap_length - 1] = taps[tap_length - 1] * in[i];
      out[k][i] = next_delay_block[k][0];
      temp = current_delay_block[k];
      current_delay_block = next_delay_block;
      next_delay_block[k] = temp;
    }
  }
}

void speech_enhancement(float *data_in, size_t in_len, float *data_out, float fsamp, noise_management *nm) {
  // Parameters for initialization
  float prob = nm->prob;
  float peak_init = nm->peak_init;
  float npow_init = nm->npow_init;
  float valley_init = nm->valley_init;

  float xpow[in_len];
  float npow[in_len];
  float vpow[in_len];
  float peak[in_len];
  float valley[in_len];
  float gain[in_len];

  float n1;
  float p;
  float b;

  int ntype = NTYPE;
  int stype = STYPE;
  float sparam = SPRAM;

  peak[0] = (peak_init == 0) ? array_mean(data_in, in_len) : peak_init;
  valley[0] = (valley_init == 0) ? peak[0] : valley_init;

  float xabs;

  for (size_t n = 1; n < in_len; n++) {
    xabs = fabsf(data_in[n]);
    /* Peak detect */
    if (xabs >= peak[n - 1])
        peak[n] = aLP * peak[n - 1] + (1 - aLP) * xabs;
    else
        peak[n] = rLP * peak[n - 1];

    /* Valley detect */

    if (xabs <= valley[n - 1])
        valley[n] = aLPv * valley[n - 1] + (1 - aLPv) * xabs;
    else
        valley[n] = valley[n - 1] / rLPv;
  }

  array_square(peak, xpow, in_len);

  /* Noise power estimation */

  npow[0] = (npow_init == 0) ? xpow[0] : npow_init;
  array_square(valley, vpow, in_len);

  if (ntype == 1) {
      /* Power estimation using limits on change (ref: Arslan et al.) */
      for (size_t n = 1; n < in_len; n++) {
          n1 = (xpow[n] < a1 * npow[n - 1]) ? xpow[n] : a1 * npow[n - 1];
          npow[n] = (n1 > a2 * npow[n - 1]) ? n1 : a2 * npow[n - 1];
      }

  } else if (ntype == 2) {
      /* Noise power estimation using the weighted averaging of Hirsch and Ehrlicher */
      for (size_t n = 1; n < in_len; n++) {
          /* Update noise average if new signal sample is close to the noise */
          if (xpow[n] < bHE * npow[n - 1])
              npow[n] = aHE * npow[n - 1] + (1 - aHE) * xpow[n];
              /* Otherwise keep the previous noise estimate */
          else
              npow[n] = npow[n - 1];

      }
  } else if (ntype == 3) {
      /* Noise power estimation using MCRA of Cohen and Berdugo */
      // prob = 0.5; // Prob of first sample being speech
      for (size_t n = 1; n < in_len; n++) {
          // P[speech] for this sample
          if (xpow[n] > delta * vpow[n])
              p = 1;
          else
              p = 0;
          prob = aCB * prob + (1 - aCB) * p; //Smooth P[speech]

          b = bCB + (1 - bCB) * prob; //Convert P[speech] into time constant

          npow[n] = b * npow[n - 1] + (1 - b) * xpow[n]; //Average the noise
          //printf("%f\n",npow[n]);
      }
  }

  /* Spectral subtraction gain (linear scale) */
  if (stype == 1)
      /* oversubtraction */
  {
      for (size_t n = 0; n < in_len; n++) {
          gain[n] = (xpow[n] - npow[n] * sparam + 0.000001f) / (xpow[n] + 0.000001f);
          if (gain[n] <= 0 || isnan(gain[n]))
            gain[n] = 1E-30;
          // printf("%f\n", gain[n]);
          data_out[n] = gain[n] * data_in[n];
      }
  } else if (stype == 0) {
      for (size_t n = 0; n < in_len; n++)
          data_out[n] = data_in[n];

  } else {
//      printf("Wrong data_in for Spectral subtraction\n");
  }

  nm->peak_init = peak[in_len - 1];
  nm->npow_init = npow[in_len - 1];
  nm->valley_init = valley[in_len - 1];
}

void noise_management_init(size_t num_per_frame, size_t sample_rate)
{
  nm = (noise_management **)malloc(sizeof(noise_management *) * NUM_BANDS);
  for (size_t i = 0; i < NUM_BANDS; i++) {
    nm[i] = (noise_management *)malloc(sizeof(noise_management));
    nm[i]->npow_init = 0;
    nm[i]->peak_init = 0;
    nm[i]->prob = 0.5;
    nm[i]->valley_init = 0;
  }
  in_float = (float *)calloc(num_per_frame, sizeof(float));
  out_float = (float *)calloc(num_per_frame, sizeof(float));
  out_filter = (float **)calloc(NUM_BANDS, sizeof(float *));
  for (size_t i = 0; i <  NUM_BANDS; i++) {
    out_filter[i] = (float *)calloc(num_per_frame, sizeof(float));
  }
  outSE = (float**) malloc(sizeof(float*) * NUM_BANDS);
  for (int i = 0; i < NUM_BANDS; i++) {
    outSE[i] = (float*) malloc(sizeof(float) * num_per_frame);
  }
  outSE_sum = (float *)malloc(num_per_frame * sizeof(float));
  current_delay_block = (float**) malloc(sizeof(float*) * NUM_BANDS);
  for (int i = 0; i < NUM_BANDS; i++) {
    current_delay_block[i] = (float*) malloc(sizeof(float) * BAND_FILT_LEN);
  }
  next_delay_block = (float**) malloc(sizeof(float*) * NUM_BANDS);
  for (int i = 0; i < NUM_BANDS; i++) {
    next_delay_block[i] = (float*) malloc(sizeof(float) * BAND_FILT_LEN);
  }

  att = 3;
  aLP = expf(-1.0f / (0.001f * att * sample_rate));
  rel = 50;
  rLP = expf(-1.0f / (0.001f * rel * sample_rate));
  aLPv = expf(-1.0f / (0.01f * sample_rate));
  rLPv = expf(-1.0f / (0.1f * sample_rate));
  inc = 10;
  dec = -25;
  lin_inc = powf(10.0f, inc / 20.0f);
  lin_dec = powf(10.0f, dec / 20.0f);
  a1 = pow(lin_inc, 1 / sample_rate);
  a2 = pow(lin_dec, 1 / sample_rate);
  tau = 100;
  aHE = exp(-1.0f / (0.001f * tau * sample_rate));
  bHE = 2;
  delta = 10;
  tau = 10;
  aCB = exp(-1.0f / (0.001f * tau * sample_rate));
  tau = 200;
  bCB = exp(-1.0f / (0.001f * tau * sample_rate));
}

void noise_management_process(int16_t *in, int16_t *out, size_t num_per_frame, size_t sample_rate)
{
  arm_q15_to_float(in, in_float, num_per_frame);
  filter_process(in_float, out_filter, num_per_frame);
  for (int i = 0; i < NUM_BANDS; i++) {
    speech_enhancement(out_filter[i], num_per_frame, outSE[i], sample_rate, nm[i]);
  }
  memset(outSE_sum, 0, num_per_frame * sizeof(float));
  for (size_t j = 0; j < NUM_BANDS; j++) {
    for (size_t i = 0; i < num_per_frame; i++) {
      outSE_sum[i] += outSE[j][i];
    }
  }
  arm_float_to_q15(outSE_sum, out, num_per_frame);
}
