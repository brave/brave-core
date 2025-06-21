/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_key.h"
#include "ui/color/color_provider_manager.h"
#include "ui/color/color_provider_source.h"
#include "ui/native_theme/test_native_theme.h"

using brave_shields::features::kBraveDarkModeBlock;

constexpr char kEmbeddedTestServerDirectory[] = "dark_mode_block";
constexpr char kMatchDarkModeFormatString[] =
    "window.matchMedia('(prefers-color-scheme: %s)').matches;";

class BraveDarkModeFingerprintProtectionTest : public InProcessBrowserTest {
 public:
  BraveDarkModeFingerprintProtectionTest() {
    feature_list_.InitAndEnableFeature(kBraveDarkModeBlock);
  }

  class BraveContentBrowserClientWithWebTheme
      : public BraveContentBrowserClient {
   public:
    explicit BraveContentBrowserClientWithWebTheme(const ui::NativeTheme* theme)
        : theme_(theme) {}

   protected:
    const ui::NativeTheme* GetWebTheme() const override { return theme_; }

   private:
    const raw_ptr<const ui::NativeTheme, DanglingUntriaged> theme_;
  };

  class MockColorProviderSource : public ui::ColorProviderSource {
   public:
    explicit MockColorProviderSource(bool is_dark) {
      key_.color_mode = is_dark ? ui::ColorProviderKey::ColorMode::kDark
                                : ui::ColorProviderKey::ColorMode::kLight;
    }
    MockColorProviderSource(const MockColorProviderSource&) = delete;
    MockColorProviderSource& operator=(const MockColorProviderSource&) = delete;
    ~MockColorProviderSource() override = default;

    // ui::ColorProviderSource:
    const ui::ColorProvider* GetColorProvider() const override {
      return &provider_;
    }
    ui::ColorProviderKey GetColorProviderKey() const override { return key_; }

    ui::RendererColorMap GetRendererColorMap(
        ui::ColorProviderKey::ColorMode color_mode,
        ui::ColorProviderKey::ForcedColors forced_colors) const override {
      auto key = GetColorProviderKey();
      key.color_mode = color_mode;
      key.forced_colors = forced_colors;
      ui::ColorProvider* color_provider =
          ui::ColorProviderManager::Get().GetColorProviderFor(key);
      CHECK(color_provider);
      return ui::CreateRendererColorMap(*color_provider);
    }

   private:
    ui::ColorProvider provider_;
    ui::ColorProviderKey key_;
  };

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content::SetBrowserClientForTesting(
        new BraveContentBrowserClientWithWebTheme(&test_theme_));

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

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

  void SetDarkMode(bool dark_mode) {
    test_theme_.SetDarkMode(dark_mode);
    browser()
        ->tab_strip_model()
        ->GetActiveWebContents()
        ->SetColorProviderSource(dark_mode ? &dark_color_provider_source_
                                           : &light_color_provider_source_);
    browser()
        ->tab_strip_model()
        ->GetActiveWebContents()
        ->OnWebPreferencesChanged();
  }

  bool IsReportingDarkMode() {
    bool light_mode_result =
        content::EvalJs(contents(),
                        base::StringPrintf(kMatchDarkModeFormatString, "light"))
            .ExtractBool();

    if (!light_mode_result) {
      // Sanity check to make sure that 'dark' is reported for
      // prefers-color-scheme when 'light' was not found before.
      EXPECT_EQ(true, content::EvalJs(contents(),
                                      base::StringPrintf(
                                          kMatchDarkModeFormatString, "dark")));

      // Report dark mode.
      return true;
    }

    // Report light mode.
    return false;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 protected:
  ui::TestNativeTheme test_theme_;
  MockColorProviderSource dark_color_provider_source_{true};
  MockColorProviderSource light_color_provider_source_{false};

 private:
  GURL top_level_page_url_;
  GURL dark_mode_url_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveDarkModeFingerprintProtectionTest,
                       PrefersColorSchemeBehavior) {
  SetDarkMode(false);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), dark_mode_url()));
  ASSERT_FALSE(IsReportingDarkMode());
  
  SetDarkMode(true);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), dark_mode_url()));
  ASSERT_FALSE(IsReportingDarkMode());
}

IN_PROC_BROWSER_TEST_F(BraveDarkModeFingerprintProtectionTest,
                       SettingsPagesExempt) {
  // On settings page should get dark mode (exempt from blocking)
  SetDarkMode(true);
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("brave://settings")));
  ASSERT_TRUE(IsReportingDarkMode());
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

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), dark_mode_url()));
  ASSERT_TRUE(IsReportingDarkMode());

  SetDarkMode(false);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), dark_mode_url()));
  ASSERT_FALSE(IsReportingDarkMode());
}
