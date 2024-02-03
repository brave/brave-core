/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VERSION_INFO_VERSION_INFO_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VERSION_INFO_VERSION_INFO_H_

#include <string_view>

#include "brave/components/version_info/version_info_values.h"

#define GetProductNameAndVersionForUserAgent \
  GetProductNameAndVersionForUserAgent_Unused

#include "src/components/version_info/version_info.h"  // IWYU pragma: export
#undef GetProductNameAndVersionForUserAgent

namespace version_info {

constexpr std::string GetProductNameAndVersionForUserAgent() {
  return "Chrome/" + std::string(constants::kBraveChromiumVersion);
}

}  // namespace version_info

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VERSION_INFO_VERSION_INFO_H_
