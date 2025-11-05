/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/path_service.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/brave_color_mixer.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/themes/test/theme_service_changed_waiter.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest-spi.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"
#include "ui/native_theme/os_settings_provider.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/common/extensions/extension_constants.h"
#include "components/crx_file/crx_verifier.h"
#include "extensions/browser/crx_file_info.h"
#include "extensions/browser/extension_dialog_auto_confirm.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/test_extension_registry_observer.h"
#endif

class BraveThemeServiceTest : public InProcessBrowserTest {
 public:
  BraveThemeServiceTest() = default;
  ~BraveThemeServiceTest() override = default;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  bool UsingCustomTheme(const ThemeService& theme_service) const {
    return !theme_service.UsingSystemTheme() &&
           !theme_service.UsingDefaultTheme();
  }

  base::FilePath GetTestDataDir() {
    base::FilePath test_data_dir;
    base::ScopedAllowBlockingForTesting allow_blocking;
    CHECK(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));
    return test_data_dir;
  }

  void InstallExtension(const char* filename) {
    base::FilePath path =
        GetTestDataDir().AppendASCII("extensions").AppendASCII(filename);

    extensions::TestExtensionRegistryObserver observer(
        extensions::ExtensionRegistry::Get(browser()->profile()));

    auto installer =
        extensions::CrxInstaller::CreateSilent(browser()->profile());
    installer->set_allow_silent_install(true);
    installer->set_was_triggered_by_user_download();
    installer->set_creation_flags(extensions::Extension::FROM_WEBSTORE);

    installer->InstallCrxFile(
        extensions::CRXFileInfo(path, crx_file::VerifierFormat::CRX3));

    observer.WaitForExtensionLoaded();
  }
#endif
};

namespace {

class TestNativeThemeObserver : public ui::NativeThemeObserver {
 public:
  TestNativeThemeObserver() = default;
  ~TestNativeThemeObserver() override = default;

  MOCK_METHOD1(OnNativeThemeUpdated, void(ui::NativeTheme*));
};

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
  auto test_theme_color = kColorForTest;

  // Test light theme
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT,
            dark_mode::GetActiveBraveDarkModeType());

  const ui::ColorProvider* color_provider =
      ThemeServiceFactory::GetForProfile(profile)->GetColorProvider();
  EXPECT_EQ(kLightColorForTest, color_provider->GetColor(test_theme_color));

  // Test dark theme
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK,
            dark_mode::GetActiveBraveDarkModeType());

  color_provider =
      ThemeServiceFactory::GetForProfile(profile)->GetColorProvider();
  EXPECT_EQ(kDarkColorForTest, color_provider->GetColor(test_theme_color));

  // Test dark theme private
  Profile* profile_private =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  const ui::ColorProvider* color_provider_private =
      ThemeServiceFactory::GetForProfile(profile_private)->GetColorProvider();
  // Private color mixer overrides are not loaded because there's no theme.
  EXPECT_EQ(kDarkColorForTest,
            color_provider_private->GetColor(test_theme_color));
}

// Test whether appropriate native/web theme observer is called when brave theme
// is changed.
IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, ThemeObserverTest) {
  // Initially set to light.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  // Check theme observer is called twice by changing theme.
  // One for changing to dark and the other for changing to light.
  TestNativeThemeObserver native_theme_observer;
  EXPECT_CALL(native_theme_observer,
              OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForNativeUi()))
      .Times(2);
  ui::NativeTheme::GetInstanceForNativeUi()->AddObserver(
      &native_theme_observer);

  TestNativeThemeObserver web_theme_observer;
  EXPECT_CALL(web_theme_observer,
              OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForWeb()))
      .Times(2);

  ui::NativeTheme::GetInstanceForWeb()->AddObserver(&web_theme_observer);

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

  ui::NativeTheme::GetInstanceForNativeUi()->RemoveObserver(
      &native_theme_observer);
  ui::NativeTheme::GetInstanceForWeb()->RemoveObserver(&web_theme_observer);
}

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, SystemThemeChangeTest) {
  ui::NativeTheme::PreferredColorScheme initial_mode =
      ui::NativeTheme::GetInstanceForNativeUi()->preferred_color_scheme();

  // Change to light.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  EXPECT_EQ(ui::NativeTheme::GetInstanceForNativeUi()->preferred_color_scheme(),
            ui::NativeTheme::PreferredColorScheme::kLight);

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
  EXPECT_EQ(ui::NativeTheme::GetInstanceForNativeUi()->preferred_color_scheme(),
            ui::NativeTheme::PreferredColorScheme::kDark);

  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
  EXPECT_EQ(ui::NativeTheme::GetInstanceForNativeUi()->preferred_color_scheme(),
            ui::NativeTheme::PreferredColorScheme::kLight);

  ASSERT_TRUE(dark_mode::SystemDarkModeEnabled());
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT);
  EXPECT_EQ(
      initial_mode,
      ui::NativeTheme::GetInstanceForNativeUi()->preferred_color_scheme());
}

// Check some colors from color provider pipeline.
IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, ColorProviderTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* cp = browser_view->GetColorProvider();
  SkColor frame_active_color = cp->GetColor(ui::kColorFrameActive);
  SkColor material_frame_color = cp->GetColor(ui::kColorSysHeader);
  EXPECT_EQ(material_frame_color, frame_active_color);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Check frame color is not ours when theme extension is installed.
  ThemeService* theme_service =
      ThemeServiceFactory::GetForProfile(browser()->profile());
  test::ThemeServiceChangedWaiter waiter(theme_service);

  EXPECT_FALSE(UsingCustomTheme(*theme_service));
  InstallExtension("theme.crx");
  waiter.WaitForThemeChanged();
  EXPECT_TRUE(UsingCustomTheme(*theme_service));

  cp = browser_view->GetColorProvider();
  frame_active_color = cp->GetColor(ui::kColorFrameActive);
  EXPECT_NE(material_frame_color, frame_active_color);
#endif

  auto* private_browser = CreateIncognitoBrowser();
  browser_view = BrowserView::GetBrowserViewForBrowser(private_browser);
  cp = browser_view->GetColorProvider();
  frame_active_color = cp->GetColor(ui::kColorFrameActive);
  EXPECT_EQ(kPrivateFrame, frame_active_color);
}

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, NonNormalWindowDarkModeTest) {
  // Check non-normal window's color provider mode is dark.
  profiles::SwitchToGuestProfile();
  Browser* guest_browser = ui_test_utils::WaitForBrowserToOpen();
  ASSERT_TRUE(guest_browser);
  ASSERT_TRUE(guest_browser->profile()->IsGuestSession());
  ASSERT_FALSE(guest_browser->profile()->IsIncognitoProfile());
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(guest_browser);
  auto* browser_widget = browser_view->GetWidget();
  auto key = browser_widget->GetColorProviderKeyForTesting();
  EXPECT_EQ(ui::ColorProviderKey::ColorMode::kDark, key.color_mode);

  auto* private_browser = CreateIncognitoBrowser(browser()->profile());
  ASSERT_TRUE(private_browser);
  ASSERT_TRUE(private_browser->profile()->IsIncognitoProfile());
  browser_view = BrowserView::GetBrowserViewForBrowser(private_browser);
  browser_widget = browser_view->GetWidget();
  key = browser_widget->GetColorProviderKeyForTesting();
  EXPECT_EQ(ui::ColorProviderKey::ColorMode::kDark, key.color_mode);

#if BUILDFLAG(ENABLE_TOR)
  Browser* tor_browser =
      TorProfileManager::SwitchToTorProfile(browser()->profile());
  ASSERT_TRUE(tor_browser);
  ASSERT_TRUE(tor_browser->profile()->IsIncognitoProfile());
  browser_view = BrowserView::GetBrowserViewForBrowser(tor_browser);
  browser_widget = browser_view->GetWidget();
  key = browser_widget->GetColorProviderKeyForTesting();
  EXPECT_EQ(ui::ColorProviderKey::ColorMode::kDark, key.color_mode);
#endif
}
