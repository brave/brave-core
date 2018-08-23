/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_

#include "chrome/browser/themes/theme_service.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class BraveThemeService : public ThemeService {
 public:
  enum BraveThemeType {
    BRAVE_THEME_TYPE_DEFAULT,  // Choose theme by channel
    BRAVE_THEME_TYPE_DARK,     // Use dark theme regardless of channel
    BRAVE_THEME_TYPE_LIGHT,    // Use light theme regardless of channel
  };

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
  static int GetBraveThemeType(Profile* profile);
  static void SetBraveThemeType(Profile* profile, BraveThemeType type);

  BraveThemeService() = default;
  ~BraveThemeService() override = default;

 protected:
  // ThemeService overrides:
  SkColor GetDefaultColor(int id, bool incognito) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveThemeService);
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
