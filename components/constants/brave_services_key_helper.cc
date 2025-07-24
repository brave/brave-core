/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/brave_services_key_helper.h"

#include "base/strings/pattern.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave {

inline constexpr char kBraveProxyPattern[] = "*.brave.com";
inline constexpr char kBraveSoftwareProxyPattern[] = "*.bravesoftware.com";

bool ShouldAddBraveServicesKeyHeader(const GURL& url) {
  return url.SchemeIs(url::kHttpsScheme) &&
      (base::MatchPattern(url.host(), kBraveProxyPattern) ||
       base::MatchPattern(url.host(), kBraveSoftwareProxyPattern));
}

}  // namespace brave
