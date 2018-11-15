/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
#define BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_

#include <memory>
#include <string>

#include "chrome/browser/themes/theme_service.h"
#include "components/prefs/pref_member.h"

namespace extensions {
class BraveThemeEventRouter;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

enum BraveThemeType {
  BRAVE_THEME_TYPE_DEFAULT,  // Choose theme by channel
  BRAVE_THEME_TYPE_DARK,     // Use dark theme regardless of channel
  BRAVE_THEME_TYPE_LIGHT,    // Use light theme regardless of channel
};

class BraveThemeService : public ThemeService {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
  static BraveThemeType GetUserPreferredBraveThemeType(Profile* profile);
  static std::string GetStringFromBraveThemeType(BraveThemeType type);
  static BraveThemeType GetActiveBraveThemeType(Profile* profile);

  BraveThemeService();
  ~BraveThemeService() override;

  // ThemeService overrides:
  void Init(Profile* profile) override;

 protected:
  // ThemeService overrides:
  SkColor GetDefaultColor(int id, bool incognito) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveThemeAPIBrowserTest, BraveThemeEventRouterTest);
  // Own |mock_router|.
  void SetBraveThemeEventRouterForTesting(
      extensions::BraveThemeEventRouter* mock_router);

  void OnPreferenceChanged(const std::string& pref_name);

  IntegerPrefMember brave_theme_type_pref_;

  std::unique_ptr<extensions::BraveThemeEventRouter> brave_theme_event_router_;

  DISALLOW_COPY_AND_ASSIGN(BraveThemeService);
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
