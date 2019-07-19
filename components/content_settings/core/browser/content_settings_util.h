/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_UTIL_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_UTIL_H_

#include <string>

#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

class HostContentSettingsMap;

namespace content_settings {

bool GetDefaultFromResourceIdentifier(const std::string& resource_identifier,
                                      const GURL& primary_url,
                                      const GURL& secondary_url);

bool IsAllowContentSetting(HostContentSettingsMap* content_settings,
                           const GURL& primary_url,
                           const GURL& secondary_url,
                           ContentSettingsType setting_type,
                           const std::string& resource_identifier);

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_UTIL_H_
