/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_GET_GUEST_VIEW_DEFAULT_CONTENT_SETTING_RULES                    \
  rules->fingerprinting_rules.push_back(ContentSettingPatternSource(          \
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(), \
      content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW),         \
      content_settings::ProviderType::kNone, incognito));                     \
  rules->brave_shields_rules.push_back(ContentSettingPatternSource(           \
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(), \
      content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW),         \
      content_settings::ProviderType::kNone, incognito));

#include "src/chrome/browser/content_settings/page_specific_content_settings_delegate.cc"

#undef BRAVE_GET_GUEST_VIEW_DEFAULT_CONTENT_SETTING_RULES
