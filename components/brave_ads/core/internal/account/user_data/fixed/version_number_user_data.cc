/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/version_number_user_data.h"

#include <string_view>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"

namespace brave_ads {

namespace {
constexpr std::string_view kVersionNumberKey = "versionNumber";
}  // namespace

base::Value::Dict BuildVersionNumberUserData() {
  std::vector<std::string> parts =
      base::SplitString(GetBrowserVersionNumber(), ".", base::KEEP_WHITESPACE,
                        base::SPLIT_WANT_ALL);
  parts.reserve(4);

  parts.resize(4, "0");
  parts[2] = "0";
  parts[3] = "0";

  return base::Value::Dict().Set(kVersionNumberKey,
                                 base::JoinString(parts, "."));
}

}  // namespace brave_ads
