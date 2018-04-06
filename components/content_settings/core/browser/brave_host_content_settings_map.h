/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_HOST_CONTENT_SETTINGS_MAP_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_HOST_CONTENT_SETTINGS_MAP_H_

#include "components/content_settings/core/browser/host_content_settings_map.h"

class BraveHostContentSettingsMap : public HostContentSettingsMap {
 public:
   BraveHostContentSettingsMap(PrefService* prefs,
                               bool is_incognito_profile,
                               bool is_guest_profile,
                               bool store_last_modified);
 private:
   void InitializeFingerprintingContentSetting();
   void InitializeReferrerContentSetting();
   void InitializeCookieContentSetting();
   ~BraveHostContentSettingsMap() override;
};

#endif // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_HOST_CONTENT_SETTINGS_MAP_H_
