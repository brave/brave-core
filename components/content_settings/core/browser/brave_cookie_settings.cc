/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"

#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace content_settings {

using namespace net::registry_controlled_domains;

BraveCookieSettings::BraveCookieSettings(
    HostContentSettingsMap* host_content_settings_map,
    PrefService* prefs,
    const char* extension_scheme)
    : CookieSettings(host_content_settings_map, prefs, extension_scheme)
{ }

BraveCookieSettings::~BraveCookieSettings() { }

void BraveCookieSettings::GetCookieSetting(const GURL& url,
    const GURL& first_party_url,
    content_settings::SettingSource* source,
    ContentSetting* cookie_setting) const {
  GetCookieSetting(url,
    first_party_url,
    first_party_url,
    source,
    cookie_setting);
}

void BraveCookieSettings::GetCookieSetting(const GURL& url,
    const GURL& first_party_url,
    const GURL& tab_url,
    content_settings::SettingSource* source,
    ContentSetting* cookie_setting) const {
  DCHECK(cookie_setting);
  CookieSettings::GetCookieSetting(url, first_party_url, source,
      cookie_setting);

  if (*cookie_setting == CONTENT_SETTING_BLOCK) {
    *cookie_setting = CONTENT_SETTING_BLOCK;
    return;
  }

  GURL primary_brave_url = tab_url == GURL("about:blank") ?
      first_party_url : tab_url;

  // Check the Brave shields setting, if it is off, just return without
  // blocking anything.
  ContentSetting brave_shields_setting =
      host_content_settings_map_->GetContentSetting(
          primary_brave_url, GURL(),
          CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields);
  if (brave_shields_setting == CONTENT_SETTING_BLOCK) {
    *cookie_setting = CONTENT_SETTING_ALLOW;
    return;
  }

  // First party setting of block means always block everything
  ContentSetting brave_1p_setting = host_content_settings_map_->GetContentSetting(
      primary_brave_url, GURL("https://firstParty/"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  if (brave_1p_setting == CONTENT_SETTING_BLOCK) {
    *cookie_setting = CONTENT_SETTING_BLOCK;
    return;
  }

  // Third party setting of allow, means always allow everything
  ContentSetting brave_3p_setting =
      host_content_settings_map_->GetContentSetting(
          primary_brave_url, GURL(),
          CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  if (brave_3p_setting == CONTENT_SETTING_ALLOW) {
    *cookie_setting = CONTENT_SETTING_ALLOW;
    return;
  }

  // Otherwise determine based on if the cookie is third-party or not.
  // Empty first-party URL indicates a first-party request, so use the
  // previously obtained cookie_setting in that case.
  if (!first_party_url.is_empty()) {
    if (SameDomainOrHost(url, first_party_url, INCLUDE_PRIVATE_REGISTRIES)) {
      *cookie_setting = brave_1p_setting;
    } else {
      *cookie_setting = brave_3p_setting == CONTENT_SETTING_DEFAULT ?
          CONTENT_SETTING_BLOCK : brave_3p_setting;
    }
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

}  // namespace content_settings
