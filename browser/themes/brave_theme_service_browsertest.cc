/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"
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
void SetBraveThemeType(Profile* profile, BTS::BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}
}  // namespace

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, BraveThemeChangeTest) {
  Profile* profile = browser()->profile();
  const SkColor light_frame_color = SkColorSetRGB(0xD8, 0xDE, 0xE1);
  const SkColor dark_frame_color = SkColorSetRGB(0x58, 0x5B, 0x5E);

  // Check default type is set initially.
  EXPECT_EQ(BTS::BRAVE_THEME_TYPE_DEFAULT, BTS::GetBraveThemeType(profile));

  const ui::ThemeProvider& tp = ThemeService::GetThemeProviderForProfile(profile);
  SetBraveThemeType(browser()->profile(), BTS::BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(BTS::BRAVE_THEME_TYPE_LIGHT, BTS::GetBraveThemeType(profile));
  EXPECT_EQ(light_frame_color, tp.GetColor(ThemeProperties::COLOR_FRAME));

  SetBraveThemeType(browser()->profile(), BTS::BRAVE_THEME_TYPE_DARK);
  EXPECT_EQ(BTS::BRAVE_THEME_TYPE_DARK, BTS::GetBraveThemeType(profile));
  EXPECT_EQ(dark_frame_color, tp.GetColor(ThemeProperties::COLOR_FRAME));
}
