/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_PREF_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_PREF_SERVICE_H_

#include <string>

#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class HostContentSettingsMap;
class PrefService;

namespace brave_shields {

// sync brave plugin cookie settings with chromium cookie prefs
class CookiePrefService : public KeyedService,
                          public content_settings::Observer {
 public:
  explicit CookiePrefService(HostContentSettingsMap* host_content_settings_map,
                             PrefService* prefs,
                             PrefService* local_state);
  CookiePrefService(const CookiePrefService&) = delete;
  CookiePrefService& operator=(const CookiePrefService&) = delete;
  ~CookiePrefService() override;

 private:
  class Lock {
   public:
    Lock();
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    ~Lock();
    bool Try();
    void Release();

   private:
    bool locked_;
  };

  void OnPreferenceChanged();

  // content_settings::Observer overrides:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

  Lock lock_;
  HostContentSettingsMap* host_content_settings_map_;
  PrefService* prefs_;
  PrefService* local_state_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_PREF_SERVICE_H_
