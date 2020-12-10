#include "arm_math.h"
#include "arm_const_structs.h"

// Global variables for FFT Bin
uint32_t fftSize = 1024;
uint32_t ifftFlag = 0;
uint32_t doBitReverse = 1;

/**
 * read a test audio
 * get fft
 * @return
 */
void
arm_fft_q15 (short *in, short *out)
{
  int index = 0;
  int i;
  while (1)
    {
      short tmp[1024];
      for (i = index; i < index + fftSize; i++)
	{
	  tmp[i - index] = in[i];
	}

      short input[2048];
      short output[1024];

      for (i = 0; i < fftSize; i++)
	{
	  input[2 * i] = tmp[i];
	  input[2 * i + 1] = 0;
	}

      arm_cfft_instance_q15 const *instance = &arm_cfft_sR_q15_len1024;
      arm_cfft_q15 (instance, input, ifftFlag, doBitReverse);
      arm_cmplx_mag_q15 (input, output, fftSize);

      for (i = 0; i < fftSize; i++)
	{
	  out[i + index] = output[i];
	}

      index += 1024;
    }

}

//void
//getFFT ()
//{
//
//  // read a pcm file
//  FILE *f;
//  f = fopen ("C:\\Users\\123\\Desktop\\output.pcm", "rb");
//
//  while (1)
//    {
//      short tmp[1024];
//      fread (tmp, sizeof(short), 1024, f);
//      fprintf (stdout, "%d\n", tmp[0]);
//      if (feof(f))
//	{
//	  break;
//	}
//
//      short input[2048];
//      short output[1024];
//
//      int i;
//      for (i = 0; i < fftSize; i++)
//	{
//	  output[2 * i] = tmp[i];
//	  output[2 * i + 1] = 0;
//	}
//
//      arm_cfft_instance_q15 const *instance = &arm_cfft_sR_q15_len1024;
//      arm_cfft_q15 (instance, input, ifftFlag, doBitReverse);
//
//      arm_cmplx_mag_q15 (input, output, fftSize);
//
//    }
//
//  fclose (f);
//  return 0;
//  arm_cfft_f32(&arm_cfft_sR_f32_len1024, tmp, ifftFlag, doBitReverse);
//
//}
