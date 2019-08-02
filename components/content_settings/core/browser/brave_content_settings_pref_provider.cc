/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "components/content_settings/core/browser/content_settings_pref.h"
#include "components/content_settings/core/browser/website_settings_registry.h"

namespace content_settings {

BravePrefProvider::BravePrefProvider(PrefService* prefs,
                                     bool off_the_record,
                                     bool store_last_modified)
    : PrefProvider(prefs, off_the_record, store_last_modified) {
  brave_pref_change_registrar_.Init(prefs_);

  WebsiteSettingsRegistry* website_settings =
      WebsiteSettingsRegistry::GetInstance();
  // Makes BravePrefProvder handle plugin type.
  for (const WebsiteSettingsInfo* info : *website_settings) {
    if (info->type() == CONTENT_SETTINGS_TYPE_PLUGINS) {
      content_settings_prefs_.insert(std::make_pair(
          info->type(),
          std::make_unique<ContentSettingsPref>(
              info->type(), prefs_, &brave_pref_change_registrar_,
              info->pref_name(), off_the_record_,
              base::Bind(&PrefProvider::Notify, base::Unretained(this)))));
      return;
    }
  }
}

void BravePrefProvider::ShutdownOnUIThread() {
  brave_pref_change_registrar_.RemoveAll();
  PrefProvider::ShutdownOnUIThread();
}

bool BravePrefProvider::SetWebsiteSetting(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    std::unique_ptr<base::Value>&& in_value) {
  // Flash's setting shouldn't be reached here.
  // Its content type is plugin and id is empty string.
  // One excpetion is default setting. It can be persisted.
  if (content_type == CONTENT_SETTINGS_TYPE_PLUGINS &&
      resource_identifier.empty()) {
    DCHECK(primary_pattern == ContentSettingsPattern::Wildcard() &&
           secondary_pattern == ContentSettingsPattern::Wildcard());
  }

  return PrefProvider::SetWebsiteSetting(primary_pattern, secondary_pattern,
                                         content_type, resource_identifier,
                                         std::move(in_value));
}

}  // namespace content_settings
