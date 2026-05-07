/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file cpu_device_api.cc
 */
#include <dmlc/logging.h>
#include <dmlc/thread_local.h>
#include <peli/runtime/registry.h>
#include <peli/runtime/device_api.h>
#include <cstdlib>
#include <cstring>
#include "workspace_pool.h"

#ifdef __ANDROID__
#include <android/api-level.h>
#endif

namespace peli {
namespace runtime {
class CPUDeviceAPI final : public DeviceAPI {
 public:
  void SetDevice(PELIContext ctx) final {}
  void GetAttr(PELIContext ctx, DeviceAttrKind kind, PELIRetValue* rv) final {
    if (kind == kExist) {
      *rv = 1;
    }
  }
  void* AllocDataSpace(PELIContext ctx,
                       size_t nbytes,
                       size_t alignment,
                       PELIType type_hint) final {
    void* ptr;
#if _MSC_VER
    ptr = _aligned_malloc(nbytes, alignment);
    if (ptr == nullptr) throw std::bad_alloc();
#elif defined(_LIBCPP_SGX_CONFIG) || (defined(__ANDROID__) && __ANDROID_API__ < 16)
    ptr = memalign(alignment, nbytes);
    if (ptr == nullptr) throw std::bad_alloc();
#else
    // posix_memalign is available in android ndk since __ANDROID_API__ >= 16
    int ret = posix_memalign(&ptr, alignment, nbytes);
    if (ret != 0) throw std::bad_alloc();
#endif
    return ptr;
  }

  void FreeDataSpace(PELIContext ctx, void* ptr) final {
#if _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
  }

  void CopyDataFromTo(const void* from,
                      size_t from_offset,
                      void* to,
                      size_t to_offset,
                      size_t size,
                      PELIContext ctx_from,
                      PELIContext ctx_to,
                      PELIType type_hint,
                      PELIStreamHandle stream) final {
    memcpy(static_cast<char*>(to) + to_offset,
           static_cast<const char*>(from) + from_offset,
           size);
  }

  void StreamSync(PELIContext ctx, PELIStreamHandle stream) final {
  }

  void* AllocWorkspace(PELIContext ctx, size_t size, PELIType type_hint) final;
  void FreeWorkspace(PELIContext ctx, void* data) final;

  static const std::shared_ptr<CPUDeviceAPI>& Global() {
    static std::shared_ptr<CPUDeviceAPI> inst =
        std::make_shared<CPUDeviceAPI>();
    return inst;
  }
};

struct CPUWorkspacePool : public WorkspacePool {
  CPUWorkspacePool() :
      WorkspacePool(kDLCPU, CPUDeviceAPI::Global()) {}
};

void* CPUDeviceAPI::AllocWorkspace(PELIContext ctx,
                                   size_t size,
                                   PELIType type_hint) {
  return dmlc::ThreadLocalStore<CPUWorkspacePool>::Get()
      ->AllocWorkspace(ctx, size);
}

void CPUDeviceAPI::FreeWorkspace(PELIContext ctx, void* data) {
  dmlc::ThreadLocalStore<CPUWorkspacePool>::Get()->FreeWorkspace(ctx, data);
}

PELI_REGISTER_GLOBAL("device_api.cpu")
.set_body([](PELIArgs args, PELIRetValue* rv) {
    DeviceAPI* ptr = CPUDeviceAPI::Global().get();
    *rv = static_cast<void*>(ptr);
  });
}  // namespace runtime
}  // namespace peli
