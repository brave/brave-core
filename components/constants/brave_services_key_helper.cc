/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/brave_services_key_helper.h"

#include "brave/components/constants/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

namespace brave {

bool ShouldAddBraveServicesKeyHeader(const GURL& url) {
  static URLPattern brave_proxy_pattern(URLPattern::SCHEME_HTTPS,
                                        kBraveProxyPattern);
  static URLPattern bravesoftware_proxy_pattern(URLPattern::SCHEME_HTTPS,
                                                kBraveSoftwareProxyPattern);
  return brave_proxy_pattern.MatchesURL(url) ||
         bravesoftware_proxy_pattern.MatchesURL(url);
}

}  // namespace brave
