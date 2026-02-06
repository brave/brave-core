/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_UTILS_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_UTILS_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "components/content_settings/core/common/content_settings_enums.mojom.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace content_settings {

const brave_shields::ShieldsContentSettingsTypes&
GetShieldsContentSettingsTypes();

std::string GetShieldsContentTypeName(const ContentSettingsType& content_type);

bool IsShieldsContentSettingsType(const ContentSettingsType& content_type);

bool IsShieldsContentSettingsTypeName(const std::string& content_type_name);

std::optional<ContentSettingsPattern> ConvertPatternToWildcardSchemeAndPort(
    const ContentSettingsPattern& pattern);

std::string GetShieldsSettingUserPrefsPath(const std::string& name);

content_settings::mojom::SessionModel GetSessionModelFromDictionary(
    const base::DictValue& dict,
    const char* key);

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_UTILS_H_
