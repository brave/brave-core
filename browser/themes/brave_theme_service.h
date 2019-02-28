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
  // DEFAULT acs two wasy depends on system theme mode.
  // If system theme mode is disableld, we override it with channel based
  // policy. See GetThemeTypeBasedOnChannel(). In this case, user can see
  // two options in theme settings(dark and light).
  // Otherwise, it act like system theme mode. In this case, user can see
  // three options in theme settings(os theme, dark and light).
  BRAVE_THEME_TYPE_DEFAULT,
  BRAVE_THEME_TYPE_DARK,
  BRAVE_THEME_TYPE_LIGHT,
};

class BraveThemeService : public ThemeService {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
  static BraveThemeType GetActiveBraveThemeType(Profile* profile);
  static base::Value GetBraveThemeList();

  BraveThemeService();
  ~BraveThemeService() override;

  // ThemeService overrides:
  void Init(Profile* profile) override;

 protected:
  // ThemeService overrides:
  SkColor GetDefaultColor(int id, bool incognito) const override;

 private:
  friend class BraveThemeServiceTestWithoutSystemTheme;
  FRIEND_TEST_ALL_PREFIXES(BraveThemeAPIBrowserTest, BraveThemeEventRouterTest);
  FRIEND_TEST_ALL_PREFIXES(BraveThemeServiceTest, GetBraveThemeListTest);

  // Own |mock_router|.
  void SetBraveThemeEventRouterForTesting(
      extensions::BraveThemeEventRouter* mock_router);

  void OnPreferenceChanged(const std::string& pref_name);

  void RecoverPrefStates(Profile* profile);
  void OverrideDefaultThemeIfNeeded(Profile* profile);

  static bool SystemThemeModeEnabled();

  static bool is_test_;
  static bool use_system_theme_mode_in_test_;

  IntegerPrefMember brave_theme_type_pref_;

  std::unique_ptr<extensions::BraveThemeEventRouter> brave_theme_event_router_;

  DISALLOW_COPY_AND_ASSIGN(BraveThemeService);
};

#endif  // BRAVE_BROWSER_THEMES_BRAVE_THEME_SERVICE_H_
