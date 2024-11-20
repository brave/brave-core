/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

constexpr char kHardwareConcurrencyScript[] = "navigator.hardwareConcurrency;";
constexpr char kTitleScript[] = "document.title;";

class BraveNavigatorHardwareConcurrencyFarblingBrowserTest
    : public InProcessBrowserTest {
 public:
  BraveNavigatorHardwareConcurrencyFarblingBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {
            brave_shields::features::kBraveShowStrictFingerprintingMode,
            webcompat::features::kBraveWebcompatExceptionsService,
        },
        {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
  }

  const GURL& farbling_url() { return farbling_url_; }

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

  void EnableWebcompatException() {
    brave_shields::SetWebcompatEnabled(
        content_settings(),
        ContentSettingsType::BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY, true,
        top_level_page_url_, nullptr);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  GURL top_level_page_url_;
  GURL farbling_url_;
};

// Tests results of farbling known values
IN_PROC_BROWSER_TEST_F(BraveNavigatorHardwareConcurrencyFarblingBrowserTest,
                       FarbleNavigatorHardwareConcurrency) {
  // Farbling level: off
  // get real navigator.hardwareConcurrency
  AllowFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int real_value =
      content::EvalJs(contents(), kHardwareConcurrencyScript).ExtractInt();
  ASSERT_GE(real_value, 2);

  // Farbling level: balanced (default)
  // navigator.hardwareConcurrency should be greater than or equal to 2
  // and less than or equal to the real value
  SetFingerprintingDefault();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int fake_value =
      content::EvalJs(contents(), kHardwareConcurrencyScript).ExtractInt();
  EXPECT_GE(fake_value, 2);
  EXPECT_LE(fake_value, real_value);

  // Farbling level: maximum
  // navigator.hardwareConcurrency should be greater than or equal to 2
  // and less than or equal to 8
  BlockFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int completely_fake_value =
      content::EvalJs(contents(), kHardwareConcurrencyScript).ExtractInt();
  // For this domain (a.com) + the random seed (constant for browser tests),
  // the value will always be the same.
  EXPECT_EQ(completely_fake_value, 8);

  // Farbling level: default, but with webcompat exception enabled
  SetFingerprintingDefault();
  EnableWebcompatException();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  int real_value2 =
      content::EvalJs(contents(), kHardwareConcurrencyScript).ExtractInt();
  ASSERT_GE(real_value, real_value2);
}

IN_PROC_BROWSER_TEST_F(BraveNavigatorHardwareConcurrencyFarblingBrowserTest,
                       FarbleNavigatorHardwareConcurrencyWorkers) {
  GURL url = embedded_test_server()->GetURL(
      "a.com", "/navigator/workers-hardware-concurrency.html");
  // Farbling level: off
  // get real navigator.hardwareConcurrency
  AllowFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  // ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), ) will return before
  // our Worker has a chance to run its code to completion, so we block here
  // until document.title changes. This will happen relatively quickly if things
  // are going well inside the Worker. If the browser crashes while executing
  // the Worker code (which is what this test is really testing), then this will
  // never unblock and the entire browser test will eventually time out. Timing
  // out indicates a fatal error.
  while (content::EvalJs(contents(), kTitleScript).ExtractString() == "") {
  }
  int real_value;
  base::StringToInt(content::EvalJs(contents(), kTitleScript).ExtractString(),
                    &real_value);
  ASSERT_GE(real_value, 2);

  // Farbling level: default
  SetFingerprintingDefault();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  while (content::EvalJs(contents(), kTitleScript).ExtractString() == "") {
  }
  int fake_value;
  base::StringToInt(content::EvalJs(contents(), kTitleScript).ExtractString(),
                    &fake_value);
  EXPECT_GE(fake_value, 2);
  EXPECT_LE(fake_value, real_value);

  // Farbling level: maximum
  BlockFingerprinting();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  while (content::EvalJs(contents(), kTitleScript).ExtractString() == "") {
  }
  int completely_fake_value;
  base::StringToInt(content::EvalJs(contents(), kTitleScript).ExtractString(),
                    &completely_fake_value);
  // For this domain (a.com) + the random seed (constant for browser tests),
  // the value will always be the same.
  EXPECT_EQ(completely_fake_value, 8);

  // Farbling level: default, but with webcompat exception enabled
  // get real navigator.hardwareConcurrency
  SetFingerprintingDefault();
  EnableWebcompatException();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  while (content::EvalJs(contents(), kTitleScript).ExtractString() == "") {
  }
  int real_value2;
  base::StringToInt(content::EvalJs(contents(), kTitleScript).ExtractString(),
                    &real_value2);
  ASSERT_GE(real_value, real_value2);
}
