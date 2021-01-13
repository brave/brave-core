/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_default_provider.h"

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"

namespace content_settings {

// static
void BraveDefaultProvider::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  DefaultProvider::RegisterProfilePrefs(registry);
}

BraveDefaultProvider::BraveDefaultProvider(PrefService* prefs,
                                           bool off_the_record)
    : DefaultProvider(prefs, off_the_record) {}

BraveDefaultProvider::~BraveDefaultProvider() {}

std::unique_ptr<RuleIterator> BraveDefaultProvider::GetRuleIterator(
    ContentSettingsType content_type,
    bool off_the_record) const {
  if (content_settings::IsShieldsContentSettingsType(content_type))
    return nullptr;

  return DefaultProvider::GetRuleIterator(content_type, off_the_record);
}

}  // namespace content_settings
