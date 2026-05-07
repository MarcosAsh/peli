/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file str_util.h
 * \brief Minimum string manipulation util for runtime.
 */
#ifndef PELI_RUNTIME_STR_UTIL_H_
#define PELI_RUNTIME_STR_UTIL_H_

#include <string>
#include <vector>

namespace peli {
namespace runtime {

std::vector<std::string> SplitString(std::string const &in, char sep);

std::string GetEnvironmentVariableOrDefault(const std::string& variable_name,
                                            const std::string& default_value);

int ParseIntOrFloat(const std::string& str, int64_t& ivalue, double& fvalue);
}  // namespace runtime
}  // namespace peli
#endif  // PELI_RUNTIME_STR_UTIL_H_
