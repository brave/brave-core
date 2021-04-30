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
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
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
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        embedded_test_server()->GetURL(domain, "/"));
  }

  void BlockFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        embedded_test_server()->GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        embedded_test_server()->GetURL(domain, "/"));
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

  std::vector<int64_t> SplitStringAsInts(std::string raw_values) {
    std::vector<int64_t> results;
    for (const auto& cur : base::SplitStringPiece(
             raw_values, base::kWhitespaceASCII, base::TRIM_WHITESPACE,
             base::SPLIT_WANT_NONEMPTY)) {
      int64_t value;
      base::StringToInt64(cur, &value);
      results.push_back(value);
    }
    return results;
  }

  std::string DiffsAsString(std::vector<int64_t> real_values,
                            std::vector<int64_t> farbled_values) {
    std::string diffs;
    for (uint64_t i = 0; i < real_values.size(); i++) {
      diffs = diffs + base::NumberToString(real_values[i] - farbled_values[i]);
    }
    return diffs;
  }

 private:
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
};

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest, FarbleGetParameterWebGL) {
  std::string domain = "a.com";
  GURL url = embedded_test_server()->GetURL(domain, "/getParameter.html");
  const std::string kExpectedRandomString = "UKlSpUqV,TJEix48e";
  // Farbling level: maximum
  // WebGL getParameter of restricted values: pseudo-random data with no
  // relation to original data
  BlockFingerprinting(domain);
  NavigateToURLUntilLoadStop(url);
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), kExpectedRandomString);
  // second time, same as the first (tests that results are consistent for the
  // lifetime of a session, and that the PRNG properly resets itself at the
  // beginning of each calculation)
  NavigateToURLUntilLoadStop(url);
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), kExpectedRandomString);

  std::string actual;
  // Farbling level: balanced (default)
  // WebGL getParameter of restricted values: original data
  SetFingerprintingDefault(domain);
  NavigateToURLUntilLoadStop(url);
  actual = ExecScriptGetStr(kTitleScript, contents());

  // Farbling level: off
  // WebGL getParameter of restricted values: original data
  AllowFingerprinting(domain);
  NavigateToURLUntilLoadStop(url);
  // Since this value depends on the underlying hardware, we just test that the
  // results for "off" are the same as the results for "balanced", and that
  // they're different than the results for "maximum".
  EXPECT_EQ(ExecScriptGetStr(kTitleScript, contents()), actual);
  EXPECT_NE(kExpectedRandomString, actual);
}

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest,
                       FarbleGetParameterWebGL2) {
  const std::map<std::string, std::string> tests = {{"a.com", "101111101011"},
                                                    {"b.com", "111011111011"},
                                                    {"c.com", "101111000110"}};
  for (const auto& pair : tests) {
    std::string domain = pair.first;
    std::string expected_diff = pair.second;
    GURL url =
        embedded_test_server()->GetURL(pair.first, "/webgl2-parameters.html");

    // Farbling level: off
    // Get the actual WebGL2 parameter values.
    AllowFingerprinting(domain);
    NavigateToURLUntilLoadStop(url);
    std::vector<int64_t> real_values =
        SplitStringAsInts(ExecScriptGetStr(kTitleScript, contents()));
    ASSERT_EQ(real_values.size(), 12UL);

    // Farbling level: default
    // WebGL2 parameter values will be farbled based on session+domain keys,
    // so we get the farbled values and look at the differences.
    SetFingerprintingDefault(domain);
    NavigateToURLUntilLoadStop(url);
    std::vector<int64_t> farbled_values =
        SplitStringAsInts(ExecScriptGetStr(kTitleScript, contents()));
    ASSERT_EQ(farbled_values.size(), 12UL);
    ASSERT_EQ(DiffsAsString(real_values, farbled_values), expected_diff);
  }
}
