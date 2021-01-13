/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/content_settings/brave_global_value_map.h"

#include <memory>

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "components/content_settings/core/browser/content_settings_rule.h"
#include "components/content_settings/core/common/content_settings.h"

namespace content_settings {

BraveGlobalValueMap::BraveGlobalValueMap() {}

BraveGlobalValueMap::~BraveGlobalValueMap() {}

std::unique_ptr<RuleIterator> BraveGlobalValueMap::GetRuleIterator(
    ContentSettingsType content_type) const {
  if (content_settings::IsShieldsContentSettingsType(content_type))
    return nullptr;
  return GlobalValueMap::GetRuleIterator(content_type);
}

void BraveGlobalValueMap::SetContentSetting(ContentSettingsType content_type,
                                            ContentSetting setting) {
  return GlobalValueMap::SetContentSetting(content_type, setting);
}

ContentSetting BraveGlobalValueMap::GetContentSetting(
    ContentSettingsType content_type) const {
  return GlobalValueMap::GetContentSetting(content_type);
}

}  // namespace content_settings
