/*!
 *  Copyright (c) 2019 by Contributors if not otherwise specified
 * \file logging.cc
 * \brief Logging utils for peli
 */

#include "ffmpeg/ffmpeg_common.h"

#include <peli/runtime/registry.h>

namespace peli {
namespace runtime {

PELI_REGISTER_GLOBAL("logging._CAPI_SetLoggingLevel")
.set_body([] (PELIArgs args, PELIRetValue* rv) {
    int log_level = args[0];
    av_log_set_level(log_level);
  });

}  // namespace runtime
}  // namespace peli
