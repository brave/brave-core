/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
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
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

const char kEmbeddedTestServerDirectory[] = "webgl";
const char kTitleScript[] = "domAutomationController.send(document.title);";

class BraveWebGLFarblingBrowserTest : public InProcessBrowserTest {
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
    get_parameter_url_ =
        embedded_test_server()->GetURL("a.com", "/getParameter.html");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  const GURL& get_parameter_url() { return get_parameter_url_; }

  void AllowFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        browser()->profile(), ControlType::ALLOW, top_level_page_url_);
  }

  void BlockFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        browser()->profile(), ControlType::BLOCK, top_level_page_url_);
  }

  void SetFingerprintingDefault() {
    brave_shields::SetFingerprintingControlType(
        browser()->profile(), ControlType::DEFAULT, top_level_page_url_);
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
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

 private:
  GURL top_level_page_url_;
  GURL get_parameter_url_;
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest, FarbleGetParameter) {
  const std::string kExpectedRandomString = "kSJEiRIk,ix4cuXLF";
  // Farbling level: maximum
  // WebGL getParameter of restricted values: pseudo-random data with no
  // relation to original data
  BlockFingerprinting();
  NavigateToURLUntilLoadStop(get_parameter_url());
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), kExpectedRandomString);
  // second time, same as the first (tests that results are consistent for the
  // lifetime of a session, and that the PRNG properly resets itself at the
  // beginning of each calculation)
  NavigateToURLUntilLoadStop(get_parameter_url());
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), kExpectedRandomString);

  std::string actual;
  // Farbling level: balanced (default)
  // WebGL getParameter of restricted values: original data
  SetFingerprintingDefault();
  NavigateToURLUntilLoadStop(get_parameter_url());
  actual = ExecScriptGetStr(kTitleScript, contents());

  // Farbling level: off
  // WebGL getParameter of restricted values: original data
  AllowFingerprinting();
  NavigateToURLUntilLoadStop(get_parameter_url());
  // Since this value depends on the underlying hardware, we just test that the
  // results for "off" are the same as the results for "balanced", and that
  // they're different than the results for "maximum".
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), actual);
  EXPECT_NE(kExpectedRandomString, actual);
}
