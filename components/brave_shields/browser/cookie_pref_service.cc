/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/cookie_pref_service.h"

#include <string>

#include "base/bind.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

void SetCookieControlTypeFromPrefs(HostContentSettingsMap* map,
                                   PrefService* prefs) {
  auto control_type = ControlType::ALLOW;
  if (prefs->GetBoolean(prefs::kBlockThirdPartyCookies)) {
    control_type = ControlType::BLOCK_THIRD_PARTY;
  }

  if (IntToContentSetting(prefs->GetInteger(
          "profile.default_content_setting_values.cookies")) ==
      ContentSetting::CONTENT_SETTING_BLOCK) {
    control_type = ControlType::BLOCK;
  }

  SetCookieControlType(map, control_type, GURL());
}

void SetCookiePrefDefaults(HostContentSettingsMap* map, PrefService* prefs) {
  auto type = GetCookieControlType(map, GURL());
  prefs->SetBoolean(prefs::kBlockThirdPartyCookies,
                    type == ControlType::BLOCK_THIRD_PARTY);

  if (type == ControlType::BLOCK) {
    prefs->SetInteger("profile.default_content_setting_values.cookies",
                      CONTENT_SETTING_BLOCK);
  } else {
    int value =
        prefs->GetInteger("profile.default_content_setting_values.cookies");
    if (IntToContentSetting(value) != CONTENT_SETTING_SESSION_ONLY) {
      value = CONTENT_SETTING_ALLOW;
    }
    prefs->SetInteger("profile.default_content_setting_values.cookies", value);
  }
}

}  // namespace

CookiePrefService::Lock::Lock() : locked_(false) {}

CookiePrefService::Lock::~Lock() {}

bool CookiePrefService::Lock::Try() {
  if (locked_)
    return false;

  locked_ = true;
  return locked_;
}

void CookiePrefService::Lock::Release() {
  DCHECK(locked_);
  locked_ = false;
}

CookiePrefService::CookiePrefService(
    HostContentSettingsMap* host_content_settings_map,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map), prefs_(prefs) {
  SetCookiePrefDefaults(host_content_settings_map, prefs);
  host_content_settings_map_->AddObserver(this);
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kBlockThirdPartyCookies,
      base::BindRepeating(&CookiePrefService::OnPreferenceChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      "profile.default_content_setting_values.cookies",
      base::BindRepeating(&CookiePrefService::OnPreferenceChanged,
                          base::Unretained(this)));
}

CookiePrefService::~CookiePrefService() {
  host_content_settings_map_->RemoveObserver(this);
}

void CookiePrefService::OnPreferenceChanged() {
  if (lock_.Try()) {
    SetCookieControlTypeFromPrefs(host_content_settings_map_, prefs_);
    lock_.Release();
  }
}

void CookiePrefService::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const std::string& resource_identifier) {
  if (primary_pattern == ContentSettingsPattern::Wildcard() &&
      secondary_pattern == ContentSettingsPattern::Wildcard() &&
      content_type == ContentSettingsType::PLUGINS &&
      resource_identifier == brave_shields::kCookies) {
    if (lock_.Try()) {
      SetCookiePrefDefaults(host_content_settings_map_, prefs_);
      lock_.Release();
    }
  }
}

}  // namespace brave_shields
