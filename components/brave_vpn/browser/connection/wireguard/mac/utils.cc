/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/mac/utils.h"

#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace brave_vpn {
absl::optional<std::string> GetConfigStringValue(const std::string& name,
                                                 const std::string& config) {
  std::vector<std::string> lines = base::SplitString(
      config, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (auto line : lines) {
    std::vector<std::string> keys = base::SplitString(
        line, "=", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (keys.size() != 2) {
      continue;
    }
    if (base::ToLowerASCII(keys.front()) == base::ToLowerASCII(name)) {
      return keys.back();
    }
  }
  return absl::nullopt;
}

}  // namespace brave_vpn
