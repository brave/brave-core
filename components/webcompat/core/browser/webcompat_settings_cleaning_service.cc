// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webcompat/core/browser/webcompat_settings_cleaning_service.h"

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/webcompat/content/browser/webcompat_exceptions_service.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_partition_key.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "content/public/browser/browser_thread.h"

namespace webcompat {

namespace {

WebcompatSettingsCleaningService* singleton = nullptr;
std::vector<base::WeakPtr<HostContentSettingsMap>> settings_maps_;

void RemoveRedundantWebcompatSettingsByType(
    base::WeakPtr<HostContentSettingsMap> settings_map,
    ContentSettingsType settings_type) {
  auto* svc = webcompat::WebcompatExceptionsService::GetInstance();
  if (!svc) {
    return;
  }
  const auto& patterns = svc->GetPatterns(settings_type);
  for (const ContentSettingPatternSource& setting :
       settings_map->GetSettingsForOneType(settings_type)) {
    if (setting.source == content_settings::ProviderType::kPrefProvider) {
      const auto prefSettingValue =
          content_settings::ValueToContentSetting(setting.setting_value);
      bool patternExists = std::find(patterns.begin(), patterns.end(),
                                     setting.primary_pattern) != patterns.end();
      if ((prefSettingValue == CONTENT_SETTING_BLOCK ||
           prefSettingValue == CONTENT_SETTING_ASK) &&
          !patternExists) {
        settings_map->SetContentSettingCustomScope(
            setting.primary_pattern, ContentSettingsPattern::Wildcard(),
            settings_type, CONTENT_SETTING_DEFAULT);
      }
    }
  }
}

void RemoveRedundantWebcompatSettings(
    base::WeakPtr<HostContentSettingsMap> settings_map) {
  for (auto settings_type = ContentSettingsType::BRAVE_WEBCOMPAT_NONE;
       settings_type != ContentSettingsType::BRAVE_WEBCOMPAT_ALL;
       settings_type = static_cast<ContentSettingsType>(
           static_cast<int32_t>(settings_type) + 1)) {
    RemoveRedundantWebcompatSettingsByType(settings_map, settings_type);
  }
  RemoveRedundantWebcompatSettingsByType(
      settings_map, ContentSettingsType::BRAVE_FINGERPRINTING_V2);
}

}  // namespace

void WebcompatSettingsCleaningService::OnWebcompatRulesUpdated() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  for (auto settings_map : settings_maps_) {
    if (settings_map) {
      RemoveRedundantWebcompatSettings(settings_map);
    }
  }
}

WebcompatSettingsCleaningService::WebcompatSettingsCleaningService() {
  webcompat::WebcompatExceptionsService::AddObserver(this);
}

WebcompatSettingsCleaningService::~WebcompatSettingsCleaningService() {}

// static
WebcompatSettingsCleaningService*
WebcompatSettingsCleaningService::CreateInstance() {
  if (singleton == nullptr) {
    singleton = new WebcompatSettingsCleaningService();
  }
  return singleton;
}

// static
void WebcompatSettingsCleaningService::AddSettingsMap(
    HostContentSettingsMap* settings_map) {
  auto settings_map_weak_ptr = settings_map->GetWeakPtr();
  settings_maps_.push_back(settings_map_weak_ptr);
  RemoveRedundantWebcompatSettings(settings_map_weak_ptr);
}

}  // namespace webcompat
