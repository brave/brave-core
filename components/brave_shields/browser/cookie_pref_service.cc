/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/cookie_pref_service.h"

#include "base/notreached.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {

content_settings::CookieControlsMode ControlTypeToCookieControlsMode(
    ControlType type) {
  switch (type) {
    case ControlType::BLOCK_THIRD_PARTY:
    case ControlType::BLOCK:
      return content_settings::CookieControlsMode::kBlockThirdParty;
    default:
      return content_settings::CookieControlsMode::kOff;
  }
}

void SetCookiePrefDefaults(HostContentSettingsMap* map,
                           content_settings::CookieSettings* cookie_seetings,
                           PrefService* prefs) {
  const auto type =
      GetCookieControlType(map, cookie_seetings, GURL::EmptyGURL());

  prefs->SetInteger(prefs::kCookieControlsMode,
                    static_cast<int>(ControlTypeToCookieControlsMode(type)));

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

CookiePrefService::CookiePrefService(
    HostContentSettingsMap* host_content_settings_map,
    content_settings::CookieSettings* cookie_settings,
    PrefService* prefs) {
  DCHECK(host_content_settings_map);
  DCHECK(cookie_settings);
  DCHECK(prefs);
  SetCookiePrefDefaults(host_content_settings_map, cookie_settings, prefs);
}

CookiePrefService::~CookiePrefService() = default;

}  // namespace brave_shields
