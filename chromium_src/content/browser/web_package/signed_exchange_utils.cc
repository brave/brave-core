/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/web_package/signed_exchange_utils.h"

namespace content::signed_exchange_utils {

bool IsSignedExchangeReportingForDistributorsEnabled() {
  return false;
}

}  // namespace content::signed_exchange_utils

#define IsSignedExchangeReportingForDistributorsEnabled \
  IsSignedExchangeReportingForDistributorsEnabled_Chromium
#include "src/content/browser/web_package/signed_exchange_utils.cc"
#undef IsSignedExchangeReportingForDistributorsEnabled
