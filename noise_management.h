/*
 * noise_management.h
 *
 *  Created on: 2020年12月10日
 *      Author: 123
 */

#ifndef NOISE_MANAGEMENT_H_
#define NOISE_MANAGEMENT_H_

#include "arm_math.h"
#include "stddef.h"
#include "stdlib.h"

void noise_management_init(size_t num_per_frame, size_t sample_rate);
void noise_management_process(int16_t *in, int16_t *out, size_t num_per_frame, size_t sample_rate);

#endif /* NOISE_MANAGEMENT_H_ */
