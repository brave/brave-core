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

void SetCookiePrefDefaults(HostContentSettingsMap* map,
                           content_settings::CookieSettings* cookie_seetings,
                           PrefService* prefs) {
  using content_settings::CookieControlsMode;

  const auto cookie_controls_mode =
      static_cast<content_settings::CookieControlsMode>(
          prefs->GetInteger(prefs::kCookieControlsMode));
  switch (cookie_controls_mode) {
    case CookieControlsMode::kOff:
    case CookieControlsMode::kBlockThirdParty:
      // Leave valid value as it is.
      break;
    default:
      // Change kIncognitoOnly and potentially broken value to kBlockThirdParty.
      prefs->SetInteger(prefs::kCookieControlsMode,
                        static_cast<int>(CookieControlsMode::kBlockThirdParty));
      break;
  }

  const auto type =
      GetCookieControlType(map, cookie_seetings, GURL::EmptyGURL());

  if (type == ControlType::BLOCK) {
    prefs->SetInteger("profile.default_content_setting_values.cookies",
                      CONTENT_SETTING_BLOCK);
  } else {
    const int value =
        prefs->GetInteger("profile.default_content_setting_values.cookies");
    if (IntToContentSetting(value) != CONTENT_SETTING_SESSION_ONLY) {
      prefs->SetInteger("profile.default_content_setting_values.cookies",
                        CONTENT_SETTING_ALLOW);
    }
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
