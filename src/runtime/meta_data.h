/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file meta_data.h
 * \brief Meta data related utilities
 */
#ifndef PELI_RUNTIME_META_DATA_H_
#define PELI_RUNTIME_META_DATA_H_

#include <dmlc/json.h>
#include <dmlc/io.h>
#include <peli/runtime/packed_func.h>
#include <string>
#include <vector>
#include "runtime_base.h"

namespace peli {
namespace runtime {

/*! \brief function information needed by device */
struct FunctionInfo {
  std::string name;
  std::vector<PELIType> arg_types;
  std::vector<std::string> thread_axis_tags;

  void Save(dmlc::JSONWriter *writer) const;
  void Load(dmlc::JSONReader *reader);
  void Save(dmlc::Stream *writer) const;
  bool Load(dmlc::Stream *reader);
};
}  // namespace runtime
}  // namespace peli

namespace dmlc {
DMLC_DECLARE_TRAITS(has_saveload, ::peli::runtime::FunctionInfo, true);
}  // namespace dmlc
#endif  // PELI_RUNTIME_META_DATA_H_
