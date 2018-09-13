/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_H_

#include "components/content_settings/core/browser/cookie_settings.h"

namespace content_settings {

class BraveCookieSettings : public CookieSettings {
 public:
  BraveCookieSettings(HostContentSettingsMap* host_content_settings_map,
                      PrefService* prefs,
                      const char* extension_scheme = kDummyExtensionScheme);
  void GetCookieSetting(const GURL& url,
                        const GURL& first_party_url,
                        content_settings::SettingSource* source,
                        ContentSetting* cookie_setting) const override;
  // For an iframe that tries to set a cookie, the first_party_url comes in as
  // from the content_browser_client, so we need to pass in the tab_url as well
  // so we can get proper shield override settings.
  void GetCookieSetting(const GURL& url,
                        const GURL& first_party_url,
                        const GURL& tab_url,
                        content_settings::SettingSource* source,
                        ContentSetting* cookie_setting) const;
  bool IsCookieAccessAllowed(const GURL& url,
                             const GURL& first_party_url,
                             const GURL& tab_url) const;
 protected:
  ~BraveCookieSettings() override;
  DISALLOW_COPY_AND_ASSIGN(BraveCookieSettings);
};

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_H_
