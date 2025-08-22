/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_AD_BLOCK_ONLY_MODE_CONTENT_SETTINGS_UTILS_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_AD_BLOCK_ONLY_MODE_CONTENT_SETTINGS_UTILS_H_

#include "components/content_settings/core/common/content_settings_types.h"

namespace content_settings {

class OriginValueMap;

bool IsAdBlockOnlyModeType(ContentSettingsType content_type,
                           bool is_off_the_record);

void SetAdBlockOnlyModeRules(OriginValueMap& ad_block_only_mode_rules);

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_AD_BLOCK_ONLY_MODE_CONTENT_SETTINGS_UTILS_H_
