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
// Copied from theme_properties.cc
// TODO: Need to share same constants.
const SkColor kDarkFrame = SkColorSetRGB(0x22, 0x22, 0x22);
const SkColor kLightFrame = SkColorSetRGB(0xd5, 0xd9, 0xdc);

void SetBraveThemeType(Profile* profile, BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}
}  // namespace

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, BraveThemeChangeTest) {
  Profile* profile = browser()->profile();

  // Check default type is set initially.
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DEFAULT, BTS::GetUserPreferredBraveThemeType(profile));

  const ui::ThemeProvider& tp = ThemeService::GetThemeProviderForProfile(profile);
  SetBraveThemeType(browser()->profile(), BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_LIGHT, BTS::GetUserPreferredBraveThemeType(profile));
  EXPECT_EQ(kLightFrame, tp.GetColor(ThemeProperties::COLOR_FRAME));

  SetBraveThemeType(browser()->profile(), BraveThemeType::BRAVE_THEME_TYPE_DARK);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DARK, BTS::GetUserPreferredBraveThemeType(profile));
  EXPECT_EQ(kDarkFrame, tp.GetColor(ThemeProperties::COLOR_FRAME));
}
