/* vim: set tabstop=4:softtabstop=4:shiftwidth=4:noexpandtab */

#pragma once

#ifndef _OPHTOOLS_H_
#define _OPHTOOLS_H_

#include <limits.h>
#include <stdint.h>
#include <string.h>

#define memzero(A, B)	memset(A, 0, B)


/*
static inline int32_t oph_2s16_to_s32(int16_t* _array,
				      unsigned int _index1,
				      unsigned int _index2) __attribute__((always_inline));
static inline int32_t oph_2s16_to_s32(int16_t* _array,
				      unsigned int _index1,
				      unsigned int _index2)
{
	return ((int32_t)_array[_index2] << (CHAR_BIT * sizeof(int32_t) / 2)) | ((uint16_t)_array[_index1]);
}
 */
#endif /* _OPHTOOLS_H_ */

