/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/content_settings/renderer/content_settings_agent_impl.h"

#include "src/components/content_settings/renderer/content_settings_agent_impl.cc"

namespace content_settings {

ContentSetting GetContentSettingFromRulesImpl(
    const ContentSettingsForOneType& rules,
    const GURL& secondary_url) {
  return GetContentSettingFromRules(rules, secondary_url);
}

bool ContentSettingsAgentImpl::HasContentSettingsRules() const {
  return content_setting_rules_.get();
}

}  // namespace content_settings
