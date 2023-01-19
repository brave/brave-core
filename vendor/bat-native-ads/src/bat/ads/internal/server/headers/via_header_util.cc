/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/headers/via_header_util.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "bat/ads/sys_info.h"

namespace ads::server {

namespace {
constexpr int kVersion = 1;
}  // namespace

std::string BuildViaHeader() {
  return base::StringPrintf(
      "Via: 1.%d brave, 1.1 ads-serve.brave.com (Apache/1.%d)",
      SysInfo().is_uncertain_future ? 1 : 0, kVersion);
}

}  // namespace ads::server
