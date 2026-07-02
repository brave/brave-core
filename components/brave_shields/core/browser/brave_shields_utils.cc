// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"

#include <optional>
#include <string>
#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom-data-view.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/content_settings/core/common/content_settings_util.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_enums.mojom-data-view.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/base/features.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

void RecordShieldsSettingChanged(PrefService* local_state) {
  ::brave_shields::MaybeRecordShieldsUsageP3A(
      ::brave_shields::kChangedPerSiteShields, local_state);
}

}  // namespace

ContentSettingsPattern GetPatternFromURL(const GURL& url) {
  return content_settings::CreateHostPattern(url);
}

bool IsBraveShieldsManaged(PrefService* prefs,
                           HostContentSettingsMap* map,
                           GURL url) {
  DCHECK(prefs);
  DCHECK(map);
  content_settings::SettingInfo info;
  map->GetWebsiteSetting(url, url, ContentSettingsType::BRAVE_SHIELDS, &info);
  return info.source == content_settings::SettingSource::kPolicy;
}

bool IsShowStrictFingerprintingModeEnabled() {
  return base::FeatureList::IsEnabled(
      features::kBraveShowStrictFingerprintingMode);
}

void SetWebcompatEnabled(HostContentSettingsMap* map,
                         ContentSettingsType webcompat_settings_type,
                         bool enabled,
                         const GURL& url,
                         PrefService* local_state) {
  DCHECK(map);

  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    return;
  }

  auto primary_pattern = GetPatternFromURL(url);
  if (!primary_pattern.IsValid()) {
    return;
  }

  ContentSetting setting =
      enabled ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  map->SetContentSettingCustomScope(primary_pattern,
                                    ContentSettingsPattern::Wildcard(),
                                    webcompat_settings_type, setting);
  RecordShieldsSettingChanged(local_state);
}

bool IsWebcompatEnabled(HostContentSettingsMap* map,
                        ContentSettingsType webcompat_settings_type,
                        const GURL& url) {
  DCHECK(map);

  if (!url.SchemeIsHTTPOrHTTPS() && !url.is_empty()) {
    return false;
  }

  ContentSetting setting =
      map->GetContentSetting(url, url, webcompat_settings_type);

  return setting == CONTENT_SETTING_ALLOW;
}

bool IsDeveloperModeEnabled(PrefService* profile_state) {
  return profile_state->GetBoolean(prefs::kAdBlockDeveloperMode);
}

void SetAllowElementBlockerInPrivateModeEnabled(PrefService* local_state,
                                                bool value) {
  local_state->SetBoolean(prefs::kAllowElementBlockerInPrivateMode, value);
}

bool GetAllowElementBlockerInPrivateModeEnabled(PrefService* local_state) {
  return local_state->GetBoolean(prefs::kAllowElementBlockerInPrivateMode);
}

}  // namespace brave_shields
