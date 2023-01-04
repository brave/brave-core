/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_PARSING_RESULT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_PARSING_RESULT_H_

#include <memory>
#include <string>

namespace ads::resource {

template <typename T>
struct ParsingResult {
  std::string error_message;
  std::unique_ptr<T> resource;
};

template <typename T>
using ParsingResultPtr = std::unique_ptr<ParsingResult<T>>;

}  // namespace ads::resource

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_PARSING_RESULT_H_
