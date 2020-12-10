/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include "noise_management.h"

#define SAMPLE_RATE (32000)

#define FRAME_LEN_10MS
#define FRAME_LEN_1MS

#ifdef FRAME_LEN_10MS
#define FRAME_SIZE (SAMPLE_RATE / 100)
#elif defined(FRAME_LEN_1MS)
#define FRAME_SIZE (SAMPLE_RATE / 1000)
#endif

int main ()
{
  puts("hello");

  short in[FRAME_SIZE];
  FILE *f1, *fout;

  f1 = fopen("moontech_mono_32k.pcm", "rb");
  fout = fopen("moontech_mono_32k_output.pcm", "wb");
  noise_management_init(FRAME_SIZE, SAMPLE_RATE);
  while (1) {
    short out[FRAME_SIZE];
    fread(in, sizeof(short), FRAME_SIZE, f1);
    noise_management_process(in, out, FRAME_SIZE, SAMPLE_RATE);
    fwrite(out, sizeof(short), FRAME_SIZE, fout);
    if (feof(f1)) break;
  }
  puts("done");
  fclose(f1);
  fclose(fout);
  return 0;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
