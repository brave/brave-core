/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"

#include "base/bind.h"
#include "brave/common/pref_names.h"
#include "brave/common/shield_exceptions.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/content_settings/core/browser/content_settings_util.h"
#include "components/prefs/pref_service.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

using namespace net::registry_controlled_domains;  // NOLINT

namespace content_settings {

namespace {

bool ShouldBlockCookie(bool allow_brave_shields,
                       bool allow_1p_cookies,
                       bool allow_3p_cookies,
                       const GURL& main_frame_url,
                       const GURL& url,
                       bool allow_google_auth) {
  // shields settings only apply to http/https
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }

  if (!allow_brave_shields) {
    return false;
  }

  // If 1p cookies are not allowed, then we just want to block everything.
  if (!allow_1p_cookies) {
    return true;
  }

  // If 3p is allowed, we have nothing extra to block
  if (allow_3p_cookies) {
    return false;
  }

  // If it is whitelisted, we shouldn't block
  if (brave::IsWhitelistedCookieException(main_frame_url,
                                          url,
                                          allow_google_auth))
    return false;

  // Same TLD+1 whouldn't set the referrer
  return !SameDomainOrHost(url, main_frame_url, INCLUDE_PRIVATE_REGISTRIES);
}

}  // namespace

BraveCookieSettings::BraveCookieSettings(
    HostContentSettingsMap* host_content_settings_map,
    PrefService* prefs,
    const char* extension_scheme)
    : CookieSettings(host_content_settings_map, prefs, extension_scheme),
      allow_google_auth_(prefs->GetBoolean(kGoogleLoginControlType)) {
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(
      kGoogleLoginControlType,
      base::BindRepeating(&BraveCookieSettings::OnAllowGoogleAuthChanged,
                          base::Unretained(this)));
}

BraveCookieSettings::~BraveCookieSettings() {}

void BraveCookieSettings::GetCookieSetting(
    const GURL& url,
    const GURL& first_party_url,
    content_settings::SettingSource* source,
    ContentSetting* cookie_setting) const {
  GetCookieSetting(url, first_party_url, first_party_url, source,
                   cookie_setting);
}

void BraveCookieSettings::GetCookieSetting(
    const GURL& url,
    const GURL& first_party_url,
    const GURL& tab_url,
    content_settings::SettingSource* source,
    ContentSetting* cookie_setting) const {
  DCHECK(cookie_setting);

  GURL main_frame_url =
      (tab_url == GURL("about:blank") || tab_url.is_empty() ? first_party_url
                                                            : tab_url);

  if (main_frame_url.is_empty())
    main_frame_url = url;

  bool allow_brave_shields =
      IsAllowContentSetting(host_content_settings_map_.get(),
                            main_frame_url,
                            main_frame_url,
                            CONTENT_SETTINGS_TYPE_PLUGINS,
                            brave_shields::kBraveShields);

  bool allow_1p_cookies =
      IsAllowContentSetting(host_content_settings_map_.get(),
                            main_frame_url,
                            GURL("https://firstParty/"),
                            CONTENT_SETTINGS_TYPE_PLUGINS,
                            brave_shields::kCookies);

  bool allow_3p_cookies =
      IsAllowContentSetting(host_content_settings_map_.get(),
                            main_frame_url,
                            GURL(),
                            CONTENT_SETTINGS_TYPE_PLUGINS,
                            brave_shields::kCookies);

  if (ShouldBlockCookie(allow_brave_shields, allow_1p_cookies, allow_3p_cookies,
                        main_frame_url, url, allow_google_auth_)) {
    *cookie_setting = CONTENT_SETTING_BLOCK;
  } else {
    return CookieSettings::GetCookieSetting(url,
                                            first_party_url,
                                            source,
                                            cookie_setting);
  }
}

bool BraveCookieSettings::IsCookieAccessAllowed(const GURL& url,
                                                const GURL& first_party_url,
                                                const GURL& tab_url) const {
  ContentSetting setting;
  GetCookieSetting(url, first_party_url, tab_url, nullptr, &setting);
  DCHECK(setting == CONTENT_SETTING_ALLOW ||
         setting == CONTENT_SETTING_SESSION_ONLY ||
         setting == CONTENT_SETTING_DEFAULT ||
         setting == CONTENT_SETTING_BLOCK);
  return setting == CONTENT_SETTING_ALLOW ||
         setting == CONTENT_SETTING_SESSION_ONLY;
}

void BraveCookieSettings::OnAllowGoogleAuthChanged() {
  DCHECK(thread_checker_.CalledOnValidThread());
  base::AutoLock auto_lock(lock_);
  allow_google_auth_ =
      pref_change_registrar_.prefs()->GetBoolean(kGoogleLoginControlType);
}

}  // namespace content_settings
