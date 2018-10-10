/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

using BraveThemeServiceTest = InProcessBrowserTest;
using BTS = BraveThemeService;

namespace {
void SetBraveThemeType(Profile* profile, BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}
}  // namespace

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, BraveThemeChangeTest) {
  Profile* profile = browser()->profile();
  Profile* profile_private = profile->GetOffTheRecordProfile();

  const ui::ThemeProvider& tp = ThemeService::GetThemeProviderForProfile(profile);
  const ui::ThemeProvider& tp_private = ThemeService::GetThemeProviderForProfile(profile_private);

  auto test_theme_property = BraveThemeProperties::COLOR_FOR_TEST;

  // Check default type is set initially.
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DEFAULT, BTS::GetUserPreferredBraveThemeType(profile));
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DEFAULT, BTS::GetUserPreferredBraveThemeType(profile_private));

  // Test light theme
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_LIGHT, BTS::GetUserPreferredBraveThemeType(profile));
  EXPECT_EQ(BraveThemeProperties::kLightColorForTest, tp.GetColor(test_theme_property));

  // Test light theme private
  SetBraveThemeType(profile_private, BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_LIGHT, BTS::GetUserPreferredBraveThemeType(profile_private));
  EXPECT_EQ(BraveThemeProperties::kPrivateColorForTest, tp_private.GetColor(test_theme_property));


  // Test dark theme
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_DARK);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DARK, BTS::GetUserPreferredBraveThemeType(profile));
  EXPECT_EQ(BraveThemeProperties::kDarkColorForTest, tp.GetColor(test_theme_property));

  // Test dark theme private
  SetBraveThemeType(profile_private, BraveThemeType::BRAVE_THEME_TYPE_DARK);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DARK, BTS::GetUserPreferredBraveThemeType(profile_private));
  EXPECT_EQ(BraveThemeProperties::kPrivateColorForTest, tp_private.GetColor(test_theme_property));
}
