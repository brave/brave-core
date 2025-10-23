/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/path_service.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/themes/pref_names.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/themes/test/theme_service_changed_waiter.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest-spi.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"

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

  PrefService* local_state() { return g_browser_process->local_state(); }

  PrefService* profile_prefs() { return browser()->profile()->GetPrefs(); }

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

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, PRE_BraveDarkModeMigrationTest) {
  // Check on first launch.
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT,
            static_cast<dark_mode::BraveDarkModeType>(
                local_state()->GetInteger(kBraveDarkMode)));
  EXPECT_EQ(ThemeService::BrowserColorScheme::kSystem,
            static_cast<ThemeService::BrowserColorScheme>(
                profile_prefs()->GetInteger(prefs::kBrowserColorScheme)));
  EXPECT_TRUE(profile_prefs()->GetBoolean(dark_mode::kBraveDarkModeMigrated));

  // Set migration is not yet done and brave dark mode is dark for checking
  // migration at next launch.
  profile_prefs()->SetBoolean(dark_mode::kBraveDarkModeMigrated, false);
  local_state()->SetInteger(
      kBraveDarkMode,
      static_cast<int>(
          dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK));
}

IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, BraveDarkModeMigrationTest) {
  EXPECT_EQ(dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK,
            static_cast<dark_mode::BraveDarkModeType>(
                local_state()->GetInteger(kBraveDarkMode)));
  EXPECT_EQ(ThemeService::BrowserColorScheme::kDark,
            static_cast<ThemeService::BrowserColorScheme>(
                profile_prefs()->GetInteger(prefs::kBrowserColorScheme)));
  EXPECT_TRUE(profile_prefs()->GetBoolean(dark_mode::kBraveDarkModeMigrated));
}
