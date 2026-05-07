/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file peli/runtime/util.h
 * \brief Useful runtime util.
 */
#ifndef PELI_RUNTIME_UTIL_H_
#define PELI_RUNTIME_UTIL_H_

#include "c_runtime_api.h"

namespace peli {
namespace runtime {

/*!
 * \brief Check whether type matches the given spec.
 * \param t The type
 * \param code The type code.
 * \param bits The number of bits to be matched.
 * \param lanes The number of lanes in the type.
 */
inline bool TypeMatch(PELIType t, int code, int bits, int lanes = 1) {
  return t.code == code && t.bits == bits && t.lanes == lanes;
}
}  // namespace runtime
}  // namespace peli
// Forward declare the intrinsic id we need
// in structure fetch to enable stackvm in runtime
namespace peli {
namespace ir {
namespace intrinsic {
/*! \brief The kind of structure field info used in intrinsic */
enum PELIStructFieldKind : int {
  // array head address
  kArrAddr,
  kArrData,
  kArrShape,
  kArrStrides,
  kArrNDim,
  kArrTypeCode,
  kArrTypeBits,
  kArrTypeLanes,
  kArrByteOffset,
  kArrDeviceId,
  kArrDeviceType,
  kArrKindBound_,
  // PELIValue field
  kPELIValueContent,
  kPELIValueKindBound_
};
}  // namespace intrinsic
}  // namespace ir
}  // namespace peli
#endif  // PELI_RUNTIME_UTIL_H_
