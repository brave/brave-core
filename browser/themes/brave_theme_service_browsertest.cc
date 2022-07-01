/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/color/brave_color_mixer.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest-spi.h"
#include "ui/color/color_provider.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"

#if BUILDFLAG(IS_WIN)
#include "base/run_loop.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "base/time/time.h"
#include "base/win/registry.h"
#endif

using BraveThemeServiceTest = InProcessBrowserTest;

namespace {

class TestNativeThemeObserver : public ui::NativeThemeObserver {
 public:
  TestNativeThemeObserver() = default;
  ~TestNativeThemeObserver() override = default;

  MOCK_METHOD1(OnNativeThemeUpdated, void(ui::NativeTheme*));
};

#if BUILDFLAG(IS_WIN)
void RunLoopRunWithTimeout(base::TimeDelta timeout) {
  // ScopedRunLoopTimeout causes a FATAL failure on timeout though, but for us
  // the timeout means success, so turn the FATAL failure into success.
  base::RunLoop run_loop;
  base::test::ScopedRunLoopTimeout run_timeout(FROM_HERE, timeout);
  // EXPECT_FATAL_FAILURE() can only reference globals and statics.
  static base::RunLoop& static_loop = run_loop;
  EXPECT_FATAL_FAILURE(static_loop.Run(), "Run() timed out.");
}
#endif

}  // namespace

class BraveThemeServiceTestWithoutSystemTheme : public InProcessBrowserTest {
 public:
  BraveThemeServiceTestWithoutSystemTheme() {
    dark_mode::SetUseSystemDarkModeEnabledForTest(false);
  }
};

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTestWithoutSystemTheme,
                       BraveThemeChangeTest) {
  Profile* profile = browser()->profile();
  Profile* profile_private =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);

  const ui::ThemeProvider& tp =
      ThemeService::GetThemeProviderForProfile(profile);
  const ui::ThemeProvider& tp_private =
      ThemeService::GetThemeProviderForProfile(profile_private);

  auto test_theme_property = BraveThemeProperties::COLOR_FOR_TEST;

  // Test light theme
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT,
            dark_mode::GetActiveBraveDarkModeType());
  EXPECT_EQ(BraveThemeProperties::kLightColorForTest,
            tp.GetColor(test_theme_property));

  // Test dark theme
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  EXPECT_EQ(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK,
      dark_mode::GetActiveBraveDarkModeType());
  EXPECT_EQ(BraveThemeProperties::kDarkColorForTest,
            tp.GetColor(test_theme_property));

  // Test dark theme private
  EXPECT_EQ(BraveThemeProperties::kPrivateColorForTest,
            tp_private.GetColor(test_theme_property));
}

// Test whether appropriate native/web theme observer is called when brave theme
// is changed.
IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, ThemeObserverTest) {
  // Initially set to light.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  // Check theme oberver is called twice by changing theme.
  // One for changing to dark and the other for changing to light.
  TestNativeThemeObserver native_theme_observer;
  EXPECT_CALL(
      native_theme_observer,
      OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForNativeUi())).Times(2);
  ui::NativeTheme::GetInstanceForNativeUi()->AddObserver(
      &native_theme_observer);

  TestNativeThemeObserver web_theme_observer;
  EXPECT_CALL(
      web_theme_observer,
      OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForWeb())).Times(2);

  ui::NativeTheme::GetInstanceForWeb()->AddObserver(
      &web_theme_observer);

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
}

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, SystemThemeChangeTest) {
  const bool initial_mode =
      ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors();

  // Change to light.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  EXPECT_FALSE(
      ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors());

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  EXPECT_TRUE(ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors());

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  EXPECT_FALSE(
      ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors());

  if (dark_mode::SystemDarkModeEnabled()) {
    dark_mode::SetBraveDarkModeType(
        dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT);
    EXPECT_EQ(initial_mode,
              ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors());
  }
}

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, OmniboxColorTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  const int hovered = false;

  // Change to light.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  bool dark = false;
  auto* color_provider = browser_view->GetColorProvider();
  EXPECT_EQ(GetLocationBarBackground(dark, false /* incognito */, hovered),
            color_provider->GetColor(kColorOmniboxBackground));
  EXPECT_EQ(GetOmniboxResultBackground(kColorOmniboxResultsBackground, dark,
                                       false /* incognito */),
            color_provider->GetColor(kColorOmniboxResultsBackground));

  // Change to dark.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  dark = true;
  color_provider = browser_view->GetColorProvider();
  EXPECT_EQ(GetLocationBarBackground(dark, false /* incognito */, hovered),
            color_provider->GetColor(kColorOmniboxBackground));
  // Check color is different on dark mode and incognito mode.
  EXPECT_NE(GetLocationBarBackground(dark, true /* incognito */, hovered),
            color_provider->GetColor(kColorOmniboxBackground));

  EXPECT_EQ(GetOmniboxResultBackground(kColorOmniboxResultsBackground, dark,
                                       false /* incognito */),
            color_provider->GetColor(kColorOmniboxResultsBackground));
}

// Check some colors from color provider pipeline.
IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, ColorProviderTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* cp = browser_view->GetColorProvider();
  SkColor frame_active_color = cp->GetColor(ui::kColorFrameActive);
  EXPECT_TRUE(frame_active_color == kLightFrame ||
              frame_active_color == kDarkFrame);

  auto* private_browser = CreateIncognitoBrowser();
  browser_view = BrowserView::GetBrowserViewForBrowser(private_browser);
  cp = browser_view->GetColorProvider();
  frame_active_color = cp->GetColor(ui::kColorFrameActive);
  EXPECT_EQ(kPrivateFrame, frame_active_color);
}

// Some tests are failing for Windows x86 CI,
// See https://github.com/brave/brave-browser/issues/22767
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_DarkModeChangeByRegTest DISABLED_DarkModeChangeByRegTest
#else
#define MAYBE_DarkModeChangeByRegTest DarkModeChangeByRegTest
#endif
#if BUILDFLAG(IS_WIN)
IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, MAYBE_DarkModeChangeByRegTest) {
  // Test native theme notification is called properly by changing reg value.
  // This simulates dark mode setting from Windows settings.
  // And Toggle it twice from initial value to go back to initial value  because
  // reg value changes system value. Otherwise, dark mode config could be
  // changed after running this test.
  if (!ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeSupported())
    return;

  base::win::RegKey hkcu_themes_regkey;
  bool key_open_succeeded = hkcu_themes_regkey.Open(
      HKEY_CURRENT_USER,
      L"Software\\Microsoft\\Windows\\CurrentVersion\\"
      L"Themes\\Personalize",
      KEY_WRITE) == ERROR_SUCCESS;
  DCHECK(key_open_succeeded);

  DWORD apps_use_light_theme = 1;
  hkcu_themes_regkey.ReadValueDW(L"AppsUseLightTheme",
                                 &apps_use_light_theme);
  const bool initial_dark_mode = apps_use_light_theme == 0;

  // Toggle dark mode and check get notification for default type (same as...).
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT);

  apps_use_light_theme = !initial_dark_mode ? 0 : 1;
  hkcu_themes_regkey.WriteValue(L"AppsUseLightTheme", apps_use_light_theme);

  TestNativeThemeObserver native_theme_observer_for_default;
  EXPECT_CALL(
      native_theme_observer_for_default,
      OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForNativeUi())).Times(1);
  ui::NativeTheme::GetInstanceForNativeUi()->AddObserver(
      &native_theme_observer_for_default);

  // Toggle dark mode and |native_theme_observer_for_light| will not get
  // notification for light type.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  TestNativeThemeObserver native_theme_observer_for_light;
  EXPECT_CALL(
      native_theme_observer_for_light,
      OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForNativeUi())).Times(0);
  ui::NativeTheme::GetInstanceForNativeUi()->AddObserver(
      &native_theme_observer_for_light);

  apps_use_light_theme = initial_dark_mode ? 0 : 1;
  hkcu_themes_regkey.WriteValue(L"AppsUseLightTheme", apps_use_light_theme);

  // Timeout is used because we can't get notifiication with light theme.
  RunLoopRunWithTimeout(base::Milliseconds(500));
}
#endif
