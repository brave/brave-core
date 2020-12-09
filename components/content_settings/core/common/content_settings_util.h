/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTIL_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTIL_H_

#include <string>

#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace content_settings {

ContentSetting GetDefaultFromContentSettingsType(
    const ContentSettingsType& content_type,
    const GURL& primary_url,
    const GURL& secondary_url);

bool IsAllowContentSetting(const ContentSettingsForOneType& content_settings,
                           const GURL& primary_url,
                           const GURL& secondary_url,
                           const ContentSettingsType& content_type);

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_CONTENT_SETTINGS_UTIL_H_
