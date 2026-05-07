/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file runtime_base.h
 * \brief Base of all C APIs
 */
#ifndef PELI_RUNTIME_RUNTIME_BASE_H_
#define PELI_RUNTIME_RUNTIME_BASE_H_

#include <peli/runtime/c_runtime_api.h>
#include <stdexcept>

/*! \brief  macro to guard beginning and end section of all functions */
#define API_BEGIN() try {
/*! \brief every function starts with API_BEGIN();
     and finishes with API_END() or API_END_HANDLE_ERROR */
#define API_END() } catch(std::runtime_error &_except_) { return PELIAPIHandleException(_except_); } return 0;  // NOLINT(*)
/*!
 * \brief every function starts with API_BEGIN();
 *   and finishes with API_END() or API_END_HANDLE_ERROR
 *   The finally clause contains procedure to cleanup states when an error happens.
 */
#define API_END_HANDLE_ERROR(Finalize) } catch(std::runtime_error &_except_) { Finalize; return PELIAPIHandleException(_except_); } return 0; // NOLINT(*)

/*!
 * \brief handle exception throwed out
 * \param e the exception
 * \return the return value of API after exception is handled
 */
inline int PELIAPIHandleException(const std::runtime_error &e) {
  PELIAPISetLastError(e.what());
  return -1;
}

#endif  // PELI_RUNTIME_RUNTIME_BASE_H_
