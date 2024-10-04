/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

constexpr char kEmbeddedTestServerDirectory[] = "webgl";
constexpr char kTitleScript[] = "document.title";

class BraveWebGLFarblingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
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

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
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
};

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest, FarbleGetParameterWebGL) {
  std::string domain = "a.com";
  GURL url = embedded_test_server()->GetURL(domain, "/getParameter.html");
  const std::string kExpectedRandomString = "USRQv2Ep,t9e2jwYU";
  // Farbling level: maximum
  // WebGL getParameter of restricted values: pseudo-random data with no
  // relation to original data
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(),
            kExpectedRandomString);
  // second time, same as the first (tests that results are consistent for the
  // lifetime of a session, and that the PRNG properly resets itself at the
  // beginning of each calculation)
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(),
            kExpectedRandomString);

  // Farbling level: balanced (default)
  // WebGL getParameter of restricted values: original data
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual = EvalJs(contents(), kTitleScript).ExtractString();

  // Farbling level: off
  // WebGL getParameter of restricted values: original data
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  // Since this value depends on the underlying hardware, we just test that the
  // results for "off" are the same as the results for "balanced", and that
  // they're different than the results for "maximum".
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(), actual);
  EXPECT_NE(actual, kExpectedRandomString);
}

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest,
                       FarbleGetParameterWebGL2) {
  const std::map<std::string, std::string> tests = {{"a.com", "101111111100"},
                                                    {"b.com", "111110111100"},
                                                    {"c.com", "000000100101"}};
  for (const auto& pair : tests) {
    std::string domain = pair.first;
    std::string expected_diff = pair.second;
    GURL url =
        embedded_test_server()->GetURL(pair.first, "/webgl2-parameters.html");

    // Farbling level: off
    // Get the actual WebGL2 parameter values.
    AllowFingerprinting(domain);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    std::vector<int64_t> real_values =
        SplitStringAsInts(EvalJs(contents(), kTitleScript).ExtractString());
    ASSERT_EQ(real_values.size(), 12UL);

    // Farbling level: default
    // WebGL2 parameter values will be farbled based on session+domain keys,
    // so we get the farbled values and look at the differences.
    SetFingerprintingDefault(domain);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    std::vector<int64_t> farbled_values =
        SplitStringAsInts(EvalJs(contents(), kTitleScript).ExtractString());
    ASSERT_EQ(farbled_values.size(), 12UL);
    ASSERT_EQ(DiffsAsString(real_values, farbled_values), expected_diff);

    // Farbling level: default, but webcompat exception enabled
    // Get the actual WebGL2 parameter values.
    SetFingerprintingDefault(domain);
    brave_shields::SetWebcompatEnabled(
        content_settings(), ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL, true,
        url, nullptr);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    std::vector<int64_t> real_values2 =
        SplitStringAsInts(EvalJs(contents(), kTitleScript).ExtractString());
    ASSERT_EQ(real_values2.size(), 12UL);
  }
}

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest, GetSupportedExtensions) {
  std::string domain = "a.com";
  GURL url =
      embedded_test_server()->GetURL(domain, "/getSupportedExtensions.html");
  const std::string kSupportedExtensionsMax = "WEBGL_debug_renderer_info";
  // Farbling level: maximum
  // WebGL getSupportedExtensions returns abbreviated list
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(),
            kSupportedExtensionsMax);

  // Farbling level: off
  // WebGL getSupportedExtensions is real
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual = EvalJs(contents(), kTitleScript).ExtractString();
  EXPECT_NE(actual, kSupportedExtensionsMax);

  // Farbling level: balanced (default)
  // WebGL getSupportedExtensions is real
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(), actual);
}

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest, GetExtension) {
  std::string domain = "a.com";
  GURL url = embedded_test_server()->GetURL(domain, "/getExtension.html");
  const std::string kExpectedExtensionListMax = "WEBGL_debug_renderer_info";
  // Farbling level: maximum
  // WebGL getExtension returns null for most names
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(),
            kExpectedExtensionListMax);

  // Farbling level: off
  // WebGL getExtension returns real objects
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual = EvalJs(contents(), kTitleScript).ExtractString();
  EXPECT_NE(actual, kExpectedExtensionListMax);

  // Farbling level: balanced (default)
  // WebGL getExtension returns real objects
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(), actual);
}

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest, GetAttachedShaders) {
  std::string domain = "a.com";
  GURL url = embedded_test_server()->GetURL(domain, "/getAttachedShaders.html");
  // In default fingerprinting mode...
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  //... getAttachedShaders() should not be null:
  // https://github.com/brave/brave-browser/issues/37044
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(),
            "[object WebGLShader]");
}
