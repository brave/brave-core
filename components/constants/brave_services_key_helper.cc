/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/brave_services_key_helper.h"

#include "base/strings/pattern.h"
#include "brave/components/constants/network_constants.h"
#include "url/gurl.h"

namespace brave {

bool ShouldAddBraveServicesKeyHeader(const GURL& url) {
  return base::MatchPattern(url.spec(), kBraveProxyPattern) ||
         base::MatchPattern(url.spec(), kBraveSoftwareProxyPattern);
}

}  // namespace brave
