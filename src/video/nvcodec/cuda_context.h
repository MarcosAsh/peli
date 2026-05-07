/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file cuda_context.h
 * \brief CUDA Context
 */

#ifndef PELI_VIDEO_NVCODEC_CUDA_CONTEXT_H
#define PELI_VIDEO_NVCODEC_CUDA_CONTEXT_H

#include <cuda.h>

namespace peli {
namespace cuda {

class CUContext {
  public:
    CUContext();
    CUContext(CUdevice device, unsigned int flags = 0);
    CUContext(CUcontext);
    ~CUContext();

    // no copying
    CUContext(const CUContext&) = delete;
    CUContext& operator=(const CUContext&) = delete;

    CUContext(CUContext&& other);
    CUContext& operator=(CUContext&& other);

    operator CUcontext() const;

    void Push() const;
    bool Initialized() const;
  private:
    CUdevice device_;
    CUcontext context_;
    bool initialized_;
};

}  // namespace cuda
}  // namespace peli

#endif  // PELI_VIDEO_NVCODEC_CUDA_CONTEXT_H
