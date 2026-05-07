/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file c_runtime_api.cc
 * \brief Device specific implementations
 */
#include <dmlc/thread_local.h>
#include <peli/runtime/c_runtime_api.h>
#include <peli/runtime/c_backend_api.h>
#include <peli/runtime/packed_func.h>
#include <peli/runtime/module.h>
#include <peli/runtime/registry.h>
#include <peli/runtime/device_api.h>
#ifdef _LIBCPP_SGX_CONFIG
#include "sgx/trusted/runtime.h"
#endif
#include <array>
#include <algorithm>
#include <string>
#include <cstdlib>
#include "runtime_base.h"

namespace peli {
namespace runtime {

class DeviceAPIManager {
 public:
  static const int kMaxDeviceAPI = 32;
  // Get API
  static DeviceAPI* Get(const PELIContext& ctx) {
    return Get(ctx.device_type);
  }
  static DeviceAPI* Get(int dev_type, bool allow_missing = false) {
    return Global()->GetAPI(dev_type, allow_missing);
  }

 private:
  std::array<DeviceAPI*, kMaxDeviceAPI> api_;
  DeviceAPI* rpc_api_{nullptr};
  std::mutex mutex_;
  // constructor
  DeviceAPIManager() {
    std::fill(api_.begin(), api_.end(), nullptr);
  }
  // Global static variable.
  static DeviceAPIManager* Global() {
    static DeviceAPIManager inst;
    return &inst;
  }
  // Get or initialize API.
  DeviceAPI* GetAPI(int type, bool allow_missing) {
    if (type < kRPCSessMask) {
      if (api_[type] != nullptr) return api_[type];
      std::lock_guard<std::mutex> lock(mutex_);
      if (api_[type] != nullptr) return api_[type];
      api_[type] = GetAPI(DeviceName(type), allow_missing);
      return api_[type];
    } else {
      if (rpc_api_ != nullptr) return rpc_api_;
      std::lock_guard<std::mutex> lock(mutex_);
      if (rpc_api_ != nullptr) return rpc_api_;
      rpc_api_ = GetAPI("rpc", allow_missing);
      return rpc_api_;
    }
  }
  DeviceAPI* GetAPI(const std::string name, bool allow_missing) {
    std::string factory = "device_api." + name;
    auto* f = Registry::Get(factory);
    if (f == nullptr) {
      CHECK(allow_missing)
          << "Device API " << name << " is not enabled.";
      return nullptr;
    }
    void* ptr = (*f)();
    return static_cast<DeviceAPI*>(ptr);
  }
};

DeviceAPI* DeviceAPI::Get(PELIContext ctx, bool allow_missing) {
  return DeviceAPIManager::Get(
      static_cast<int>(ctx.device_type), allow_missing);
}

void* DeviceAPI::AllocWorkspace(PELIContext ctx,
                                size_t size,
                                PELIType type_hint) {
  return AllocDataSpace(ctx, size, kTempAllocaAlignment, type_hint);
}

void DeviceAPI::FreeWorkspace(PELIContext ctx, void* ptr) {
  FreeDataSpace(ctx, ptr);
}

PELIStreamHandle DeviceAPI::CreateStream(PELIContext ctx) {
  LOG(FATAL) << "Device does not support stream api.";
  return 0;
}

void DeviceAPI::FreeStream(PELIContext ctx, PELIStreamHandle stream) {
  LOG(FATAL) << "Device does not support stream api.";
}

void DeviceAPI::SyncStreamFromTo(PELIContext ctx,
                                 PELIStreamHandle event_src,
                                 PELIStreamHandle event_dst) {
  LOG(FATAL) << "Device does not support stream api.";
}
}  // namespace runtime
}  // namespace peli

using namespace peli::runtime;

struct PELIRuntimeEntry {
  std::string ret_str;
  std::string last_error;
  PELIByteArray ret_bytes;
};

typedef dmlc::ThreadLocalStore<PELIRuntimeEntry> PELIAPIRuntimeStore;

const char *PELIGetLastError() {
  return PELIAPIRuntimeStore::Get()->last_error.c_str();
}

void PELIAPISetLastError(const char* msg) {
#ifndef _LIBCPP_SGX_CONFIG
  PELIAPIRuntimeStore::Get()->last_error = msg;
#else
  sgx::OCallPackedFunc("__sgx_set_last_error__", msg);
#endif
}

int PELIModLoadFromFile(const char* file_name,
                       const char* format,
                       PELIModuleHandle* out) {
  API_BEGIN();
  Module m = Module::LoadFromFile(file_name, format);
  *out = new Module(m);
  API_END();
}

int PELIModImport(PELIModuleHandle mod,
                 PELIModuleHandle dep) {
  API_BEGIN();
  static_cast<Module*>(mod)->Import(
      *static_cast<Module*>(dep));
  API_END();
}

int PELIModGetFunction(PELIModuleHandle mod,
                      const char* func_name,
                      int query_imports,
                      PELIFunctionHandle *func) {
  API_BEGIN();
  PackedFunc pf = static_cast<Module*>(mod)->GetFunction(
      func_name, query_imports != 0);
  if (pf != nullptr) {
    *func = new PackedFunc(pf);
  } else {
    *func = nullptr;
  }
  API_END();
}

int PELIModFree(PELIModuleHandle mod) {
  API_BEGIN();
  delete static_cast<Module*>(mod);
  API_END();
}

int PELIBackendGetFuncFromEnv(void* mod_node,
                             const char* func_name,
                             PELIFunctionHandle *func) {
  API_BEGIN();
  *func = (PELIFunctionHandle)(
      static_cast<ModuleNode*>(mod_node)->GetFuncFromEnv(func_name));
  API_END();
}

void* PELIBackendAllocWorkspace(int device_type,
                               int device_id,
                               uint64_t size,
                               int dtype_code_hint,
                               int dtype_bits_hint) {
  PELIContext ctx;
  ctx.device_type = static_cast<DLDeviceType>(device_type);
  ctx.device_id = device_id;

  PELIType type_hint;
  type_hint.code = static_cast<decltype(type_hint.code)>(dtype_code_hint);
  type_hint.bits = static_cast<decltype(type_hint.bits)>(dtype_bits_hint);
  type_hint.lanes = 1;

  return DeviceAPIManager::Get(ctx)->AllocWorkspace(ctx,
                                                    static_cast<size_t>(size),
                                                    type_hint);
}

int PELIBackendFreeWorkspace(int device_type,
                            int device_id,
                            void* ptr) {
  PELIContext ctx;
  ctx.device_type = static_cast<DLDeviceType>(device_type);
  ctx.device_id = device_id;
  DeviceAPIManager::Get(ctx)->FreeWorkspace(ctx, ptr);
  return 0;
}

int PELIBackendRunOnce(void** handle,
                      int (*f)(void*),
                      void* cdata,
                      int nbytes) {
  if (*handle == nullptr) {
    *handle = reinterpret_cast<void*>(1);
    return (*f)(cdata);
  }
  return 0;
}

int PELIFuncFree(PELIFunctionHandle func) {
  API_BEGIN();
  delete static_cast<PackedFunc*>(func);
  API_END();
}

int PELIFuncCall(PELIFunctionHandle func,
                PELIValue* args,
                int* arg_type_codes,
                int num_args,
                PELIValue* ret_val,
                int* ret_type_code) {
  API_BEGIN();
  PELIRetValue rv;
  (*static_cast<const PackedFunc*>(func)).CallPacked(
      PELIArgs(args, arg_type_codes, num_args), &rv);
  // handle return string.
  if (rv.type_code() == kStr ||
     rv.type_code() == kPELIType ||
      rv.type_code() == kBytes) {
    PELIRuntimeEntry* e = PELIAPIRuntimeStore::Get();
    if (rv.type_code() != kPELIType) {
      e->ret_str = *rv.ptr<std::string>();
    } else {
      e->ret_str = rv.operator std::string();
    }
    if (rv.type_code() == kBytes) {
      e->ret_bytes.data = e->ret_str.c_str();
      e->ret_bytes.size = e->ret_str.length();
      *ret_type_code = kBytes;
      ret_val->v_handle = &(e->ret_bytes);
    } else {
      *ret_type_code = kStr;
      ret_val->v_str = e->ret_str.c_str();
    }
  } else {
    rv.MoveToCHost(ret_val, ret_type_code);
  }
  API_END();
}

int PELICFuncSetReturn(PELIRetValueHandle ret,
                      PELIValue* value,
                      int* type_code,
                      int num_ret) {
  API_BEGIN();
  CHECK_EQ(num_ret, 1);
  PELIRetValue* rv = static_cast<PELIRetValue*>(ret);
  *rv = PELIArgValue(value[0], type_code[0]);
  API_END();
}

int PELIFuncCreateFromCFunc(PELIPackedCFunc func,
                           void* resource_handle,
                           PELIPackedCFuncFinalizer fin,
                           PELIFunctionHandle *out) {
  API_BEGIN();
  if (fin == nullptr) {
    *out = new PackedFunc(
        [func, resource_handle](PELIArgs args, PELIRetValue* rv) {
          int ret = func((PELIValue*)args.values, (int*)args.type_codes, // NOLINT(*)
                         args.num_args, rv, resource_handle);
          if (ret != 0) {
            std::string err = "PELICall CFunc Error:\n";
            err += PELIGetLastError();
            throw dmlc::Error(err);
          }
        });
  } else {
    // wrap it in a shared_ptr, with fin as deleter.
    // so fin will be called when the lambda went out of scope.
    std::shared_ptr<void> rpack(resource_handle, fin);
    *out = new PackedFunc(
        [func, rpack](PELIArgs args, PELIRetValue* rv) {
          int ret = func((PELIValue*)args.values, (int*)args.type_codes, // NOLINT(*)
                         args.num_args, rv, rpack.get());
          if (ret != 0) {
            std::string err = "PELICall CFunc Error:\n";
            err += PELIGetLastError();
            throw dmlc::Error(err);
          }
      });
  }
  API_END();
}

int PELIStreamCreate(int device_type, int device_id, PELIStreamHandle* out) {
  API_BEGIN();
  PELIContext ctx;
  ctx.device_type = static_cast<DLDeviceType>(device_type);
  ctx.device_id = device_id;
  *out = DeviceAPIManager::Get(ctx)->CreateStream(ctx);
  API_END();
}

int PELIStreamFree(int device_type, int device_id, PELIStreamHandle stream) {
  API_BEGIN();
  PELIContext ctx;
  ctx.device_type = static_cast<DLDeviceType>(device_type);
  ctx.device_id = device_id;
  DeviceAPIManager::Get(ctx)->FreeStream(ctx, stream);
  API_END();
}

int PELISetStream(int device_type, int device_id, PELIStreamHandle stream) {
  API_BEGIN();
  PELIContext ctx;
  ctx.device_type = static_cast<DLDeviceType>(device_type);
  ctx.device_id = device_id;
  DeviceAPIManager::Get(ctx)->SetStream(ctx, stream);
  API_END();
}

int PELISynchronize(int device_type, int device_id, PELIStreamHandle stream) {
  API_BEGIN();
  PELIContext ctx;
  ctx.device_type = static_cast<DLDeviceType>(device_type);
  ctx.device_id = device_id;
  DeviceAPIManager::Get(ctx)->StreamSync(ctx, stream);
  API_END();
}

int PELIStreamStreamSynchronize(int device_type,
                               int device_id,
                               PELIStreamHandle src,
                               PELIStreamHandle dst) {
  API_BEGIN();
  PELIContext ctx;
  ctx.device_type = static_cast<DLDeviceType>(device_type);
  ctx.device_id = device_id;
  DeviceAPIManager::Get(ctx)->SyncStreamFromTo(ctx, src, dst);
  API_END();
}

int PELICbArgToReturn(PELIValue* value, int code) {
  API_BEGIN();
  peli::runtime::PELIRetValue rv;
  rv = peli::runtime::PELIArgValue(*value, code);
  int tcode;
  rv.MoveToCHost(value, &tcode);
  CHECK_EQ(tcode, code);
  API_END();
}

// set device api
PELI_REGISTER_GLOBAL(peli::runtime::symbol::peli_set_device)
.set_body([](PELIArgs args, PELIRetValue *ret) {
    PELIContext ctx;
    ctx.device_type = static_cast<DLDeviceType>(args[0].operator int());
    ctx.device_id = args[1];
    DeviceAPIManager::Get(ctx)->SetDevice(ctx);
  });

// set device api
PELI_REGISTER_GLOBAL("_GetDeviceAttr")
.set_body([](PELIArgs args, PELIRetValue *ret) {
    PELIContext ctx;
    ctx.device_type = static_cast<DLDeviceType>(args[0].operator int());
    ctx.device_id = args[1];

    DeviceAttrKind kind = static_cast<DeviceAttrKind>(args[2].operator int());
    if (kind == kExist) {
      DeviceAPI* api = DeviceAPIManager::Get(ctx.device_type, true);
      if (api != nullptr) {
        api->GetAttr(ctx, kind, ret);
      } else {
        *ret = 0;
      }
    } else {
      DeviceAPIManager::Get(ctx)->GetAttr(ctx, kind, ret);
    }
  });
