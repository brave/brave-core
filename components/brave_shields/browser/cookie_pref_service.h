/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_PREF_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_PREF_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"

class HostContentSettingsMap;
class PrefService;

namespace content_settings {
class CookieSettings;
}

namespace brave_shields {

// sync brave plugin cookie settings with chromium cookie prefs
class CookiePrefService : public KeyedService {
 public:
  explicit CookiePrefService(PrefService* prefs);
  CookiePrefService(const CookiePrefService&) = delete;
  CookiePrefService& operator=(const CookiePrefService&) = delete;
  ~CookiePrefService() override;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_PREF_SERVICE_H_
