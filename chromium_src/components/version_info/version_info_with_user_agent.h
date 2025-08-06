/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_VERSION_INFO_VERSION_INFO_WITH_USER_AGENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_VERSION_INFO_VERSION_INFO_WITH_USER_AGENT_H_

#include <string>

#include "brave/components/version_info/version_info_values.h"

#define GetProductNameAndVersionForUserAgent \
  GetProductNameAndVersionForUserAgent_Unused

#include <components/version_info/version_info_with_user_agent.h>  // IWYU pragma: export
#undef GetProductNameAndVersionForUserAgent

namespace version_info {

constexpr std::string GetProductNameAndVersionForUserAgent() {
  return "Chrome/" + std::string(constants::kBraveChromiumVersion);
}

}  // namespace version_info

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_VERSION_INFO_VERSION_INFO_WITH_USER_AGENT_H_
