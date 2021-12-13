/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/native_theme/test_native_theme.h"

using brave_shields::ControlType;
using brave_shields::features::kBraveDarkModeBlock;

const char kEmbeddedTestServerDirectory[] = "dark_mode_block";
const char kMatchDarkModeFormatString[] =
    "window.domAutomationController.send(window."
    "matchMedia('(prefers-color-scheme: %s)').matches)";

class BraveDarkModeFingerprintProtectionTest : public InProcessBrowserTest {
 public:
  class BraveContentBrowserClientWithWebTheme
      : public BraveContentBrowserClient {
   public:
    explicit BraveContentBrowserClientWithWebTheme(const ui::NativeTheme* theme)
        : theme_(theme) {}

   protected:
    const ui::NativeTheme* GetWebTheme() const override { return theme_; }

   private:
    const ui::NativeTheme* const theme_;
  };

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content::SetBrowserClientForTesting(
        new BraveContentBrowserClientWithWebTheme(&test_theme_));

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    dark_mode_url_ =
        embedded_test_server()->GetURL("a.com", "/dark_mode_fingerprint.html");
  }

  const GURL& dark_mode_url() { return dark_mode_url_; }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW, top_level_page_url_);
  }

  void BlockFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK, top_level_page_url_);
  }

  void SetFingerprintingDefault() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT, top_level_page_url_);
  }

  void SetDarkMode(bool dark_mode) {
    test_theme_.SetDarkMode(dark_mode);
    browser()
        ->tab_strip_model()
        ->GetActiveWebContents()
        ->OnWebPreferencesChanged();
  }

  bool IsReportingDarkMode() {
    bool light_mode_result = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        contents(), base::StringPrintf(kMatchDarkModeFormatString, "light"),
        &light_mode_result));

    if (!light_mode_result) {
      // Sanity check to make sure that 'dark' is reported for
      // prefers-color-scheme when 'light' was not found before.
      bool dark_mode_result = false;
      EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
          contents(), base::StringPrintf(kMatchDarkModeFormatString, "dark"),
          &dark_mode_result));
      EXPECT_TRUE(dark_mode_result);

      // Report dark mode.
      return true;
    }

    // Report light mode.
    return false;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateToURLUntilLoadStop(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(WaitForLoadStop(contents()));
  }

 protected:
  ui::TestNativeTheme test_theme_;

 private:
  GURL top_level_page_url_;
  GURL dark_mode_url_;
};

IN_PROC_BROWSER_TEST_F(BraveDarkModeFingerprintProtectionTest, DarkModeCheck) {
  SetDarkMode(true);

  // On fingerprinting off, should return dark mode
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_TRUE(IsReportingDarkMode());

  // On fingerprinting default, should return dark mode
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_TRUE(IsReportingDarkMode());

  // On fingerprinting block, should return light
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_FALSE(IsReportingDarkMode());
}

IN_PROC_BROWSER_TEST_F(BraveDarkModeFingerprintProtectionTest,
                       RegressionCheck) {
  SetDarkMode(false);

  // On all modes, should return light
  // Fingerprinting off
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_FALSE(IsReportingDarkMode());

  // Fingerprinting default
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_FALSE(IsReportingDarkMode());

  // Fingerprinting strict/block
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_FALSE(IsReportingDarkMode());
}

IN_PROC_BROWSER_TEST_F(BraveDarkModeFingerprintProtectionTest,
                       SettingsPagesCheck) {
  // On settings page should get dark mode with fingerprinting strict
  SetDarkMode(true);

  BlockFingerprinting();
  NavigateToURLUntilLoadStop(GURL("brave://settings"));
  ASSERT_TRUE(IsReportingDarkMode());
}

IN_PROC_BROWSER_TEST_F(BraveDarkModeFingerprintProtectionTest,
                       PrefersColorSchemeWithDefaultFingerprinting) {
  SetFingerprintingDefault();

  SetDarkMode(false);
  NavigateToURLUntilLoadStop(dark_mode_url());
  std::u16string tab_title;
  ASSERT_TRUE(ui_test_utils::GetCurrentTabTitle(browser(), &tab_title));
  EXPECT_EQ(u"light", tab_title);

  SetDarkMode(true);
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_TRUE(ui_test_utils::GetCurrentTabTitle(browser(), &tab_title));
  EXPECT_EQ(u"dark", tab_title);
}

class BraveDarkModeFingerprintProtectionFlagDisabledTest
    : public BraveDarkModeFingerprintProtectionTest {
 public:
  BraveDarkModeFingerprintProtectionFlagDisabledTest() {
    feature_list_.InitAndDisableFeature(kBraveDarkModeBlock);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveDarkModeFingerprintProtectionFlagDisabledTest,
                       WithFeatureDisabled) {
  SetDarkMode(true);

  // On fingerprinting off, should return dark mode
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_TRUE(IsReportingDarkMode());

  // On fingerprinting default, should return dark mode
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_TRUE(IsReportingDarkMode());

  // On fingerprinting block, should still return dark due to the
  // BraveDarkModeBlock feature being disabled for this test.
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(dark_mode_url());
  ASSERT_TRUE(IsReportingDarkMode());
}
