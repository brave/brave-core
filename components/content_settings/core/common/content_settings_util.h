/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTIL_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTIL_H_

#include "components/content_settings/core/common/content_settings.h"

namespace content_settings {

struct ShieldsCookiesPatterns {
  ContentSettingsPattern host_pattern;
  ContentSettingsPattern domain_pattern;
};

ShieldsCookiesPatterns CreateShieldsCookiesPatterns(const GURL& url);

// Create "*://hostname/*" patern.
ContentSettingsPattern CreateHostPattern(const GURL& url);

// Create "*://[*.]etldp1_hostname/*" patern.
ContentSettingsPattern CreateDomainPattern(const GURL& url);

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTIL_H_
