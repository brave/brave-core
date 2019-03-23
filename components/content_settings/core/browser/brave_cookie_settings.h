/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_H_

#include "components/content_settings/core/browser/cookie_settings.h"

class HostContentSettingsMap;

namespace content_settings {

class BraveCookieSettings : public CookieSettings {
 public:
  using CookieSettingsBase::IsCookieAccessAllowed;

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

  // Should be used by default to gate access to cookies and other storage APIs
  bool IsCookieAccessAllowed(const GURL& url,
                             const GURL& first_party_url,
                             const GURL& tab_url) const;

  bool ShouldStoreState(HostContentSettingsMap* map,
                             int render_process_id,
                             int render_frame_id,
                             const GURL& url,
                             const GURL& first_party_url,
                             const GURL& tab_url) const;

  bool GetAllowGoogleAuth() const {
    return allow_google_auth_;
  }

 protected:
  ~BraveCookieSettings() override;
  void OnAllowGoogleAuthChanged();

  bool allow_google_auth_;

  DISALLOW_COPY_AND_ASSIGN(BraveCookieSettings);
};

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_COOKIE_SETTINGS_H_
