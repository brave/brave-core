/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

const char kHardwareConcurrencyScript[] =
    "domAutomationController.send(navigator.hardwareConcurrency);";
const char kTitleScript[] = "domAutomationController.send(document.title);";

class BraveNavigatorHardwareConcurrencyFarblingBrowserTest
    : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
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

  template <typename T>
  int ExecScriptGetInt(const std::string& script, T* frame) {
    int value;
    EXPECT_TRUE(ExecuteScriptAndExtractInt(frame, script, &value));
    return value;
  }

  template <typename T>
  std::string ExecScriptGetStr(const std::string& script, T* frame) {
    std::string value;
    EXPECT_TRUE(ExecuteScriptAndExtractString(frame, script, &value));
    return value;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    return WaitForLoadStop(contents());
  }

 private:
  GURL top_level_page_url_;
  GURL farbling_url_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

// Tests results of farbling known values
IN_PROC_BROWSER_TEST_F(BraveNavigatorHardwareConcurrencyFarblingBrowserTest,
                       FarbleNavigatorHardwareConcurrency) {
  // Farbling level: off
  // get real navigator.hardwareConcurrency
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  int real_value = ExecScriptGetInt(kHardwareConcurrencyScript, contents());
  ASSERT_GE(real_value, 2);

  // Farbling level: balanced (default)
  // navigator.hardwareConcurrency should be greater than or equal to 2
  // and less than or equal to the real value
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(farbling_url());
  int fake_value = ExecScriptGetInt(kHardwareConcurrencyScript, contents());
  EXPECT_GE(fake_value, 2);
  EXPECT_LE(fake_value, real_value);

  // Farbling level: maximum
  // navigator.hardwareConcurrency should be greater than or equal to 2
  // and less than or equal to 8
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(farbling_url());
  int completely_fake_value =
      ExecScriptGetInt(kHardwareConcurrencyScript, contents());
  // For this domain (a.com) + the random seed (constant for browser tests),
  // the value will always be the same.
  EXPECT_EQ(completely_fake_value, 7);
}

IN_PROC_BROWSER_TEST_F(BraveNavigatorHardwareConcurrencyFarblingBrowserTest,
                       FarbleNavigatorHardwareConcurrencyWorkers) {
  GURL url = embedded_test_server()->GetURL(
      "a.com", "/navigator/workers-hardware-concurrency.html");
  // Farbling level: off
  // get real navigator.hardwareConcurrency
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(url);
  // NavigateToURLUntilLoadStop() will return before our Worker has a chance
  // to run its code to completion, so we block here until document.title
  // changes. This will happen relatively quickly if things are going well
  // inside the Worker. If the browser crashes while executing the Worker
  // code (which is what this test is really testing), then this will never
  // unblock and the entire browser test will eventually time out. Timing
  // out indicates a fatal error.
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  int real_value;
  base::StringToInt(ExecScriptGetStr(kTitleScript, contents()), &real_value);
  ASSERT_GE(real_value, 2);

  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(url);
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  int fake_value;
  base::StringToInt(ExecScriptGetStr(kTitleScript, contents()), &fake_value);
  EXPECT_GE(fake_value, 2);
  EXPECT_LE(fake_value, real_value);

  BlockFingerprinting();
  NavigateToURLUntilLoadStop(url);
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  int completely_fake_value;
  base::StringToInt(ExecScriptGetStr(kTitleScript, contents()),
                    &completely_fake_value);
  // For this domain (a.com) + the random seed (constant for browser tests),
  // the value will always be the same.
  EXPECT_EQ(completely_fake_value, 7);
}
