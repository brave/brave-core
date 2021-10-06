/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
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

const char kEmbeddedTestServerDirectory[] = "canvas";
const char kTitleScript[] = "domAutomationController.send(document.title);";
const char kExpectedImageDataHashFarblingBalanced[] = "204";
const char kExpectedImageDataHashFarblingOff[] = "0";
const char kExpectedImageDataHashFarblingMaximum[] = "204";

class BraveOffscreenCanvasFarblingBrowserTest : public InProcessBrowserTest {
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
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

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
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(BraveOffscreenCanvasFarblingBrowserTest,
                       MustNotTimeout) {
  GURL url =
      embedded_test_server()->GetURL("a.com", "/offscreen-farbling.html");
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
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "pass");

  BlockFingerprinting();
  NavigateToURLUntilLoadStop(url);
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "pass");

  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(url);
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), "pass");
}

IN_PROC_BROWSER_TEST_F(BraveOffscreenCanvasFarblingBrowserTest,
                       FarbleGetImageData) {
  GURL url = embedded_test_server()->GetURL(
      "a.com", "/offscreen-getimagedata-farbling.html");

  AllowFingerprinting();
  NavigateToURLUntilLoadStop(url);
  // wait for worker thread to complete
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()),
            kExpectedImageDataHashFarblingOff);

  BlockFingerprinting();
  NavigateToURLUntilLoadStop(url);
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()),
            kExpectedImageDataHashFarblingMaximum);

  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(url);
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()),
            kExpectedImageDataHashFarblingBalanced);

  // Turn off shields to test that the worker content settings agent
  // properly respects shields setting separately from fingerprinting
  // setting.
  brave_shields::SetBraveShieldsEnabled(content_settings(), false, url);
  NavigateToURLUntilLoadStop(url);
  while (ExecScriptGetStr(kTitleScript, contents()) == "") {
  }
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()),
            kExpectedImageDataHashFarblingOff);
}
