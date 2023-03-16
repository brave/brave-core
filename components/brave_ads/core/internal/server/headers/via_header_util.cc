/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/server/headers/via_header_util.h"

#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/sys_info.h"

namespace brave_ads::server {

namespace {
constexpr int kVersion = 1;
}  // namespace

std::string BuildViaHeader() {
  return base::StringPrintf(
      "Via: 1.%d brave, 1.1 ads-serve.brave.com (Apache/1.%d)",
      SysInfo().is_uncertain_future ? 1 : 0, kVersion);
}

}  // namespace brave_ads::server
