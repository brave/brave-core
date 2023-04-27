/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_MIGRATION_UTILS_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_MIGRATION_UTILS_H_

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_constraints.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace content_settings {
namespace migration_utils {

base::span<const ContentSettingsType> GetPreM88ShieldsContentSettingsTypes();

base::StringPiece GetPreM88ShieldsContentTypeName(
    const ContentSettingsType& content_type);

bool IsPreM88ShieldsContentSettingsTypeName(
    const std::string& content_type_name);

absl::optional<ContentSettingsPattern> ConvertPatternToWildcardSchemeAndPort(
    const ContentSettingsPattern& pattern);

std::string GetShieldsSettingUserPrefsPath(base::StringPiece name);

content_settings::SessionModel GetSessionModelFromDictionary(
    const base::Value::Dict& dict,
    const char* key);

}  // namespace migration_utils
}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_MIGRATION_UTILS_H_
