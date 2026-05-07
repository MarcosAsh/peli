/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file improc.h
 * \brief Image processing functions
 */

#ifndef PELI_IMPROC_IMPROC_H_
#define PELI_IMPROC_IMPROC_H_

#include <stdint.h>

namespace peli {
namespace cuda {

#ifdef PELI_USE_CUDA

void ProcessFrame(cudaTextureObject_t chroma, cudaTextureObject_t luma,
                  uint8_t* dst, cudaStream_t stream, uint16_t input_width, uint16_t input_height,
                  int output_width, int output_height);
#endif
}  // namespace imp
}  // namespace peli


#endif  // PELI_IMPROC_IMPROC_H_
