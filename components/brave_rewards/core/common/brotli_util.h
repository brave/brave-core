/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_BROTLI_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_BROTLI_UTIL_H_

#include <string>

#include "base/strings/string_piece.h"

namespace brave_rewards::core {
namespace util {

bool DecodeBrotliString(base::StringPiece input,
                        size_t uncompressed_size,
                        std::string* output);

bool DecodeBrotliStringWithBuffer(base::StringPiece input,
                                  size_t buffer_size,
                                  std::string* output);

}  // namespace util
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_BROTLI_UTIL_H_
