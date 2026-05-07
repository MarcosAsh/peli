/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file peli/runtime/c_runtime_api.h
 * \brief PELI runtime library.
 *
 *  The philosophy of PELI project is to customize the compilation
 *  stage to generate code that can used by other projects transparently.
 *  So this is a minimum runtime code gluing, and some limited
 *  memory management code to enable quick testing.
 *
 *  The runtime API is independent from PELI compilation stack and can
 *  be linked via libpeli_runtime.
 *
 *  The common flow is:
 *   - Use PELIFuncListGlobalNames to get global function name
 *   - Use PELIFuncCall to call these functions.
 */
#ifndef PELI_RUNTIME_C_RUNTIME_API_H_
#define PELI_RUNTIME_C_RUNTIME_API_H_

// Macros to do weak linking
#ifdef _MSC_VER
#define PELI_WEAK __declspec(selectany)
#else
#define PELI_WEAK __attribute__((weak))
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define PELI_DLL EMSCRIPTEN_KEEPALIVE
#endif

#ifndef PELI_DLL
#ifdef _WIN32
#ifdef PELI_EXPORTS
#define PELI_DLL __declspec(dllexport)
#else
#define PELI_DLL __declspec(dllimport)
#endif
#else
#define PELI_DLL __attribute__((visibility("default")))
#endif
#endif

// PELI version
#define PELI_VERSION "0.1.0"


// PELI Runtime is DLPack compatible.
#include <dlpack/dlpack.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>

/*! \brief type of array index. */
typedef int64_t peli_index_t;

/*! \brief Extension device types in PELI */
typedef enum {
  kDLAOCL = 5,
  kDLSDAccel = 6,
  kOpenGL = 11,
  // AddExtraPELIType which is not in DLPack here
} PELIDeviceExtType;

/*!
 * \brief The type code in PELIType
 * \note PELIType is used in two places.
 */
typedef enum {
  // The type code of other types are compatible with DLPack.
  // The next few fields are extension types
  // that is used by PELI API calls.
  kHandle = 3U,
  kNull = 4U,
  kPELIType = 5U,
  kPELIContext = 6U,
  kArrayHandle = 7U,
  kNodeHandle = 8U,
  kModuleHandle = 9U,
  kFuncHandle = 10U,
  kStr = 11U,
  kBytes = 12U,
  kNDArrayContainer = 13U,
  // Extension codes for other frameworks to integrate PELI PackedFunc.
  // To make sure each framework's id do not conflict, use first and
  // last sections to mark ranges.
  // Open an issue at the repo if you need a section of code.
  kExtBegin = 15U,
  kNNVMFirst = 16U,
  kNNVMLast = 20U,
  // The following section of code is used for non-reserved types.
  kExtReserveEnd = 64U,
  kExtEnd = 128U
} PELITypeCode;

/*!
 * \brief The data type used in PELI Runtime.
 *
 *  Examples
 *   - float: type_code = 2, bits = 32, lanes=1
 *   - float4(vectorized 4 float): type_code = 2, bits = 32, lanes=4
 *   - int8: type_code = 0, bits = 8, lanes=1
 *
 * \note Arguments PELI API function always takes bits=64 and lanes=1
 */
typedef DLDataType PELIType;

/*!
 * \brief The Device information, abstract away common device types.
 */
typedef DLContext PELIContext;

/*!
 * \brief The tensor array stucture to PELI API.
 */
typedef DLTensor PELIArray;

/*! \brief the array handle */
typedef PELIArray* PELIArrayHandle;

/*!
 * \brief Union type of values
 *  being passed through API and function calls.
 */
typedef union {
  int64_t v_int64;
  double v_float64;
  void* v_handle;
  const char* v_str;
  PELIType v_type;
  PELIContext v_ctx;
} PELIValue;

/*!
 * \brief Byte array type used to pass in byte array
 *  When kBytes is used as data type.
 */
typedef struct {
  const char* data;
  size_t size;
} PELIByteArray;

/*! \brief Handle to PELI runtime modules. */
typedef void* PELIModuleHandle;
/*! \brief Handle to packed function handle. */
typedef void* PELIFunctionHandle;
/*! \brief Handle to hold return value. */
typedef void* PELIRetValueHandle;
/*!
 * \brief The stream that is specific to device
 * can be NULL, which indicates the default one.
 */
typedef void* PELIStreamHandle;

/*!
 * \brief Used for implementing C API function.
 *  Set last error message before return.
 * \param msg The error message to be set.
 */
PELI_DLL void PELIAPISetLastError(const char* msg);

/*!
 * \brief return str message of the last error
 *  all function in this file will return 0 when success
 *  and -1 when an error occured,
 *  PELIGetLastError can be called to retrieve the error
 *
 *  this function is threadsafe and can be called by different thread
 *  \return error info
 */
PELI_DLL const char *PELIGetLastError(void);
/*!
 * \brief Load module from file.
 * \param file_name The file name to load the module from.
 * \param format The format of the module.
 * \param out The result module
 *
 * \return 0 when success, -1 when failure happens
 * \note The resulting module do not contain import relation.
 *  It can be reconstructed by PELIModImport.
 */
PELI_DLL int PELIModLoadFromFile(const char* file_name,
                               const char* format,
                               PELIModuleHandle* out);

/*!
 * \brief Add dep to mod's dependency.
 *  This allows functions in this module to use modules.
 *
 * \param mod The module handle.
 * \param dep The dependent module to be imported.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIModImport(PELIModuleHandle mod,
                         PELIModuleHandle dep);

/*!
 * \brief Get function from the module.
 * \param mod The module handle.
 * \param func_name The name of the function.
 * \param query_imports Whether to query imported modules
 * \param out The result function, can be NULL if it is not available.
 * \return 0 when no error is thrown, -1 when failure happens
 */
PELI_DLL int PELIModGetFunction(PELIModuleHandle mod,
                              const char* func_name,
                              int query_imports,
                              PELIFunctionHandle *out);

/*!
 * \brief Free front-end extension type resource.
 * \param handle The extension handle.
 * \param type_code The type of of the extension type.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIExtTypeFree(void* handle, int type_code);

/*!
 * \brief Free the Module
 * \param mod The module to be freed.
 *
 * \note This may not free up the module's resources.
 *  If there is active PELIFunctionHandle uses the module
 *  Or if this module is imported by another active module.
 *
 *  The all functions remains valid until PELIFuncFree is called.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIModFree(PELIModuleHandle mod);

/*!
 * \brief Free the function when it is no longer needed.
 * \param func The function handle
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIFuncFree(PELIFunctionHandle func);

/*!
 * \brief Call a Packed PELI Function.
 *
 * \param func node handle of the function.
 * \param arg_values The arguments
 * \param type_codes The type codes of the arguments
 * \param num_args Number of arguments.
 *
 * \param ret_val The return value.
 * \param ret_type_code the type code of return value.
 *
 * \return 0 when success, -1 when failure happens
 * \note PELI calls always exchanges with type bits=64, lanes=1
 *
 * \note API calls always exchanges with type bits=64, lanes=1
 *   If API call returns container handles (e.g. FunctionHandle)
 *   these handles should be managed by the front-end.
 *   The front-end need to call free function (e.g. PELIFuncFree)
 *   to free these handles.
 */
PELI_DLL int PELIFuncCall(PELIFunctionHandle func,
                        PELIValue* arg_values,
                        int* type_codes,
                        int num_args,
                        PELIValue* ret_val,
                        int* ret_type_code);

/*!
 * \brief Set the return value of PELIPackedCFunc.
 *
 *  This function is called by PELIPackedCFunc to set the return value.
 *  When this function is not called, the function returns null by default.
 *
 * \param ret The return value handle, pass by ret in PELIPackedCFunc
 * \param value The value to be returned.
 * \param type_code The type of the value to be returned.
 * \param num_ret Number of return values, for now only 1 is supported.
 */
PELI_DLL int PELICFuncSetReturn(PELIRetValueHandle ret,
                              PELIValue* value,
                              int* type_code,
                              int num_ret);

/*!
 * \brief Inplace translate callback argument value to return value.
 *  This is only needed for non-POD arguments.
 *
 * \param value The value to be translated.
 * \param code The type code to be translated.
 * \note This function will do a shallow copy when necessary.
 *
 * \return 0 when success, -1 when failure happens.
 */
PELI_DLL int PELICbArgToReturn(PELIValue* value, int code);

/*!
 * \brief C type of packed function.
 *
 * \param args The arguments
 * \param type_codes The type codes of the arguments
 * \param num_args Number of arguments.
 * \param ret The return value handle.
 * \param resource_handle The handle additional resouce handle from fron-end.
 * \return 0 if success, -1 if failure happens, set error via PELIAPISetLastError.
 * \sa PELICFuncSetReturn
 */
typedef int (*PELIPackedCFunc)(
    PELIValue* args,
    int* type_codes,
    int num_args,
    PELIRetValueHandle ret,
    void* resource_handle);

/*!
 * \brief C callback to free the resource handle in C packed function.
 * \param resource_handle The handle additional resouce handle from fron-end.
 */
typedef void (*PELIPackedCFuncFinalizer)(void* resource_handle);

/*!
 * \brief Signature for extension function declarer.
 *
 *  PELI call this function to get the extension functions
 *  The declarer will call register_func to register function and their name.
 *
 * \param register_func_handle The register function
 * \return 0 if success, -1 if failure happens
 */
typedef int (*PELIExtensionFuncDeclarer)(PELIFunctionHandle register_func_handle);

/*!
 * \brief Wrap a PELIPackedCFunc to become a FunctionHandle.
 *
 * The resource_handle will be managed by PELI API, until the function is no longer used.
 *
 * \param func The packed C function.
 * \param resource_handle The resource handle from front-end, can be NULL.
 * \param fin The finalizer on resource handle when the FunctionHandle get freed, can be NULL
 * \param out the result function handle.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIFuncCreateFromCFunc(PELIPackedCFunc func,
                                   void* resource_handle,
                                   PELIPackedCFuncFinalizer fin,
                                   PELIFunctionHandle *out);

/*!
 * \brief Register the function to runtime's global table.
 *
 * The registered function then can be pulled by the backend by the name.
 *
 * \param name The name of the function.
 * \param f The function to be registered.
 * \param override Whether allow override already registered function.
 */
PELI_DLL int PELIFuncRegisterGlobal(
    const char* name, PELIFunctionHandle f, int override);

/*!
 * \brief Get a global function.
 *
 * \param name The name of the function.
 * \param out the result function pointer, NULL if it does not exist.
 *
 * \note The function handle of global function is managed by PELI runtime,
 *  So PELIFuncFree is should not be called when it get deleted.
 */
PELI_DLL int PELIFuncGetGlobal(const char* name, PELIFunctionHandle* out);

/*!
 * \brief List all the globally registered function name
 * \param out_size The number of functions
 * \param out_array The array of function names.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIFuncListGlobalNames(int* out_size,
                                   const char*** out_array);

// Array related apis for quick proptyping
/*!
 * \brief Allocate a nd-array's memory,
 *  including space of shape, of given spec.
 *
 * \param shape The shape of the array, the data content will be copied to out
 * \param ndim The number of dimension of the array.
 * \param dtype_code The type code of the dtype
 * \param dtype_bits The number of bits of dtype
 * \param dtype_lanes The number of lanes in the dtype.
 * \param device_type The device type of context
 * \param device_id The device id of context.
 * \param out The output handle.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIArrayAlloc(const peli_index_t* shape,
                          int ndim,
                          int dtype_code,
                          int dtype_bits,
                          int dtype_lanes,
                          int device_type,
                          int device_id,
                          PELIArrayHandle* out);

/*!
 * \brief Free the PELI Array.
 * \param handle The array handle to be freed.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIArrayFree(PELIArrayHandle handle);

/*!
 * \brief Copy array data from CPU byte array.
 * \param handle The array handle.
 * \param data the data pointer
 * \param nbytes The number of bytes to copy.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIArrayCopyFromBytes(PELIArrayHandle handle,
                                  void* data,
                                  size_t nbytes);

/*!
 * \brief Copy array data to CPU byte array.
 * \param handle The array handle.
 * \param data the data pointer
 * \param nbytes The number of bytes to copy.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIArrayCopyToBytes(PELIArrayHandle handle,
                                void* data,
                                size_t nbytes);

/*!
 * \brief Copy the array, both from and to must be valid during the copy.
 * \param from The array to be copied from.
 * \param to The target space.
 * \param stream The stream where the copy happens, can be NULL.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIArrayCopyFromTo(PELIArrayHandle from,
                               PELIArrayHandle to,
                               PELIStreamHandle stream);

/*!
 * \brief Produce an array from the DLManagedTensor that shares data memory
 * with the DLManagedTensor.
 * \param from The source DLManagedTensor.
 * \param out The output array handle.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIArrayFromDLPack(DLManagedTensor* from,
                               PELIArrayHandle* out);

/*!
 * \brief Produce a DLMangedTensor from the array that shares data memory with
 * the array.
 * \param from The source array.
 * \param out The DLManagedTensor handle.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIArrayToDLPack(PELIArrayHandle from,
                             DLManagedTensor** out);

/*!
 * \brief Delete (free) a DLManagedTensor's data.
 * \param dltensor Pointer to the DLManagedTensor.
 */
PELI_DLL void PELIDLManagedTensorCallDeleter(DLManagedTensor* dltensor);

/*!
 * \brief Create a new runtime stream.
 *
 * \param device_type The device type of context
 * \param device_id The device id of context
 * \param out The new stream handle
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIStreamCreate(int device_type, int device_id, PELIStreamHandle* out);

/*!
 * \brief Free a created stream handle.
 *
 * \param device_type The device type of context
 * \param device_id The device id of context
 * \param stream The stream to be freed
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIStreamFree(int device_type, int device_id, PELIStreamHandle stream);

/*!
 * \brief Set the runtime stream of current thread to be stream.
 *  The subsequent calls to the same device_type
 *  will use the setted stream handle.
 *  The specific type of stream is runtime device dependent.
 *
 * \param device_type The device type of context
 * \param device_id The device id of context.
 * \param handle The stream handle.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELISetStream(int device_type, int device_id, PELIStreamHandle handle);

/*!
 * \brief Wait until all computations on stream completes.
 *
 * \param device_type The device type of context
 * \param device_id The device id of context.
 * \param stream The stream to be synchronized.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELISynchronize(int device_type, int device_id, PELIStreamHandle stream);

/*!
 * \brief Synchronize two streams of execution.
 *
 * \param device_type The device type of context
 * \param device_id The device id of context
 * \param src The source stream to synchronize.
 * \param dst The destination stream to synchronize.
 * \return 0 when success, -1 when failure happens
 */
PELI_DLL int PELIStreamStreamSynchronize(int device_type,
                                       int device_id,
                                       PELIStreamHandle src,
                                       PELIStreamHandle dst);

#ifdef __cplusplus
}  // PELI_EXTERN_C
#endif
#endif  // PELI_RUNTIME_C_RUNTIME_API_H_
