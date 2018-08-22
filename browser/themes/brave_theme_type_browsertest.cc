/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/theme_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/browser.h"

using BraveThemeTypeTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveThemeTypeTest, BraveThemeChangeTest) {
  Profile* profile = browser()->profile();
#if defined(OFFICIAL_BUILD)
  const SkColor light_frame_color = SkColorSetRGB(0xD8, 0xDE, 0xE1);
#endif
  const SkColor dark_frame_color = SkColorSetRGB(0x58, 0x5B, 0x5E);

  // Check default type is set initially.
  EXPECT_EQ(BRAVE_THEME_TYPE_DEFAULT, GetBraveThemeType(profile));

  const ui::ThemeProvider& tp = ThemeService::GetThemeProviderForProfile(profile);
  SetBraveThemeType(browser()->profile(), BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(BRAVE_THEME_TYPE_LIGHT, GetBraveThemeType(profile));
#if defined(OFFICIAL_BUILD)
  EXPECT_EQ(light_frame_color, tp.GetColor(ThemeProperties::COLOR_FRAME));
#else
  // Non-official build always uses dark theme.
  EXPECT_EQ(dark_frame_color, tp.GetColor(ThemeProperties::COLOR_FRAME));
#endif

  SetBraveThemeType(browser()->profile(), BRAVE_THEME_TYPE_DARK);
  EXPECT_EQ(BRAVE_THEME_TYPE_DARK, GetBraveThemeType(profile));
  EXPECT_EQ(dark_frame_color, tp.GetColor(ThemeProperties::COLOR_FRAME));
}