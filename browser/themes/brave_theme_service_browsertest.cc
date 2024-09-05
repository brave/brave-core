/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/color/brave_color_mixer.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/test/theme_service_changed_waiter.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/buildflags/buildflags.h"
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

    auto installer = extensions::CrxInstaller::CreateSilent(
        extensions::ExtensionSystem::Get(browser()->profile())
            ->extension_service());
    installer->set_allow_silent_install(true);
    installer->set_install_cause(extension_misc::INSTALL_CAUSE_USER_DOWNLOAD);
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

#if BUILDFLAG(IS_WIN)
void RunLoopRunWithTimeout(base::TimeDelta timeout) {
  // ScopedRunLoopTimeout causes a non-fatal failure on timeout but for us the
  // timeout means success, so turn the failure into success.
  base::RunLoop run_loop;
  base::test::ScopedRunLoopTimeout run_timeout(FROM_HERE, timeout);
  EXPECT_NONFATAL_FAILURE(run_loop.Run(), "Run() timed out.");
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

  // Check theme oberver is called twice by changing theme.
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

#if BUILDFLAG(IS_WIN)
// Some tests are failing for Windows x86 CI,
// See https://github.com/brave/brave-browser/issues/22767
#if defined(ARCH_CPU_X86)
#define MAYBE_DarkModeChangeByRegTest DISABLED_DarkModeChangeByRegTest
#else
#define MAYBE_DarkModeChangeByRegTest DarkModeChangeByRegTest
#endif
IN_PROC_BROWSER_TEST_F(BraveThemeServiceTest, MAYBE_DarkModeChangeByRegTest) {
  // Test native theme notification is called properly by changing reg value.
  // This simulates dark mode setting from Windows settings.
  // And Toggle it twice from initial value to go back to initial value  because
  // reg value changes system value. Otherwise, dark mode config could be
  // changed after running this test.
  if (!ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeSupported()) {
    return;
  }

  base::win::RegKey hkcu_themes_regkey;
  bool key_open_succeeded =
      hkcu_themes_regkey.Open(HKEY_CURRENT_USER,
                              L"Software\\Microsoft\\Windows\\CurrentVersion\\"
                              L"Themes\\Personalize",
                              KEY_WRITE) == ERROR_SUCCESS;
  DCHECK(key_open_succeeded);

  DWORD apps_use_light_theme = 1;
  hkcu_themes_regkey.ReadValueDW(L"AppsUseLightTheme", &apps_use_light_theme);
  const bool initial_dark_mode = apps_use_light_theme == 0;

  // Set dark mode to "Same as Windows". In this mode we want to be receiving
  // system notifications of the dark mode changes.
  dark_mode::SetBraveDarkModeType(
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT);

  {
    // Set up theme observer and toggle the system dark mode by writing to the
    // registry. We should get 2 notifications:
    //    1 for dark mode change + 1 for reduced transparency.
    // We get notifications for both because they are watching the same registry
    // key.
    TestNativeThemeObserver native_theme_observer_for_default;
    ui::NativeTheme::GetInstanceForNativeUi()->AddObserver(
        &native_theme_observer_for_default);

    EXPECT_CALL(native_theme_observer_for_default,
                OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForNativeUi()))
        .Times(2);
    apps_use_light_theme = !initial_dark_mode ? 0 : 1;
    hkcu_themes_regkey.WriteValue(L"AppsUseLightTheme", apps_use_light_theme);

    // Timeout is used to let notifications trickle in.
    RunLoopRunWithTimeout(base::Milliseconds(500));

    // Toggling dark mode to light should result in only one notification since
    // we aren't touching the registry.
    EXPECT_CALL(native_theme_observer_for_default,
                OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForNativeUi()))
        .Times(1);
    dark_mode::SetBraveDarkModeType(
        dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);

    ui::NativeTheme::GetInstanceForNativeUi()->RemoveObserver(
        &native_theme_observer_for_default);
  }

  {
    // Set up theme observer and toggle the system dark mode via the registry
    // again. We should only get 1 reduced transparency notifications this time
    // because we short circuit dark mode change system notifications when we
    // are in non-default dark mode (i.e. dark mode is set to Dark or Light, not
    // to "Same as Windows").
    TestNativeThemeObserver native_theme_observer_for_light;
    EXPECT_CALL(native_theme_observer_for_light,
                OnNativeThemeUpdated(ui::NativeTheme::GetInstanceForNativeUi()))
        .Times(1);
    ui::NativeTheme::GetInstanceForNativeUi()->AddObserver(
        &native_theme_observer_for_light);

    apps_use_light_theme = initial_dark_mode ? 0 : 1;
    hkcu_themes_regkey.WriteValue(L"AppsUseLightTheme", apps_use_light_theme);

    // Timeout is used to let notifications trickle in.
    RunLoopRunWithTimeout(base::Milliseconds(500));

    ui::NativeTheme::GetInstanceForNativeUi()->RemoveObserver(
        &native_theme_observer_for_light);
  }
}
#endif  // BUILDFLAG(IS_WIN)
