/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <optional>
#include <vector>

#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
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
#include "third_party/blink/public/common/features.h"

using brave_shields::ControlType;

namespace {
constexpr char kEmbeddedTestServerDirectory[] = "webgl";
constexpr char kTitleScript[] = "document.title";

enum class TestFarblingLevel {
  OFF = 0,
  BALANCED = 1,
  MAXIMUM = 2,
};

// Can't rely on GetFakeSupportedExtensionsForTesting since it uses
// blink::String and we can't seem to construct them in the context of
// brave_browser_tests without hitting this DCHECK
// https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/platform/wtf/allocator/partitions.h;l=72.
// This is a heuristic which allows us to compare it's one of the fake names as
// they always ends with these suffixes.
bool IsFakeExtensionName(std::string_view name) {
  return std::ranges::any_of(
      std::vector{
          "ompressor",
          "ampler",
          "lender",
      },
      [&name](const auto& farbled_endings) {
        return name.ends_with(farbled_endings);
      });
}

void VerifyBalancedFarblingExtensions(const std::string& actual_off,
                                      const std::string& actual_balanced,
                                      bool expect_farbling) {
  auto actual_extensions_list = base::SplitString(
      actual_off, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::sort(actual_extensions_list.begin(), actual_extensions_list.end());
  EXPECT_FALSE(actual_extensions_list.empty());

  auto actual_balanced_extensions_list = base::SplitString(
      actual_balanced, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  std::sort(actual_balanced_extensions_list.begin(),
            actual_balanced_extensions_list.end());
  std::vector<std::string> diff;
  std::ranges::set_difference(actual_balanced_extensions_list,
                              actual_extensions_list, std::back_inserter(diff));

  if (expect_farbling) {
    // This should contain one of the farbled values.
    ASSERT_EQ(diff.size(), 1u);
    EXPECT_TRUE(IsFakeExtensionName(diff[0])) << diff[0];
  } else {
    EXPECT_EQ(actual_balanced_extensions_list, actual_extensions_list);
  }
}

}  // namespace

class BraveWebGLFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveWebGLFarblingBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {
            brave_shields::features::kBraveShowStrictFingerprintingMode,
            webcompat::features::kBraveWebcompatExceptionsService,
        },
        {brave_shields::features::kBraveFarblingTokenReset});
  }

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

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWebGLFarblingBrowserTest,
                       FarbleGetParameterWebGL2) {
  const std::map<std::string, std::string> tests = {{"a.com", "101010000011"},
                                                    {"b.com", "100101000101"},
                                                    {"c.com", "010101100011"}};
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
    EXPECT_EQ(DiffsAsString(real_values, farbled_values), expected_diff);

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

class BraveWebGLExtensionFarblingTest
    : public BraveWebGLFarblingBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  BraveWebGLExtensionFarblingTest() {
    if (GetParam()) {
      get_parameter_feature_list_.InitWithFeatures(
          {blink::features::kWebGLBalancedFingerprintingProtection}, {});
    } else {
      get_parameter_feature_list_.InitWithFeatures(
          {}, {blink::features::kWebGLBalancedFingerprintingProtection});
    }
  }

  std::string GetExpectedString(
      TestFarblingLevel level,
      std::optional<std::string> expected_override = std::nullopt) {
    if (expected_override.has_value()) {
      return expected_override.value();
    }
    if (level == TestFarblingLevel::MAXIMUM) {
      return "uAfPPuXL,aseXyZzC";
    } else if (level == TestFarblingLevel::BALANCED) {
      if (GetParam()) {
        return "Brave,Brave";
      } else {
        ADD_FAILURE() << "Must provide a valid expected_override";
        return "";
      }
    } else {
      ADD_FAILURE() << "Must provide a valid expected_override";
      return "";
    }
  }

 private:
  base::test::ScopedFeatureList get_parameter_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveWebGLExtensionFarblingTest,
    testing::Bool(),
    [](const testing::TestParamInfo<bool>& info) {
      return info.param ? "WebGLBalancedFingerprintingProtection_Enabled"
                        : "WebGLBalancedFingerprintingProtection_Disabled";
    });

IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       FarbleVendorAndRendererDebugInfoWebGL) {
  std::string domain = "a.com";
  GURL url = embedded_test_server()->GetURL(domain, "/getParameter.html");

  // Farbling level: off
  // This is tested below in relation with "maximum" and "balanced"
  // farbling.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual_value_off =
      EvalJs(contents(), kTitleScript).ExtractString();

  // Farbling level: maximum
  // pseudo-random data with no relation to original data
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string expected_value_maximum =
      GetExpectedString(TestFarblingLevel::MAXIMUM);
  std::string actual_value_maximum =
      EvalJs(contents(), kTitleScript).ExtractString();
  EXPECT_EQ(expected_value_maximum, actual_value_maximum);
  // second time, same as the first (tests that results are consistent for the
  // lifetime of a session, and that the PRNG properly resets itself at the
  // beginning of each calculation)
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  actual_value_maximum = EvalJs(contents(), kTitleScript).ExtractString();
  EXPECT_EQ(expected_value_maximum, actual_value_maximum);
  // Check never same as the "off" state.
  EXPECT_NE(actual_value_off, actual_value_maximum);

  // Farbling level: balanced (default)
  // If feature flag was "on", we do farbling otherwise not.
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual_value_balanced =
      EvalJs(contents(), kTitleScript).ExtractString();
  auto expected_value_balanced = GetExpectedString(
      TestFarblingLevel::BALANCED, /*expected_override= */
      GetParam() ? std::nullopt : std::optional<std::string>(actual_value_off));
  EXPECT_EQ(expected_value_balanced, actual_value_balanced);
  // Check never the same as "maximum" state.
  EXPECT_NE(actual_value_balanced, actual_value_maximum);
}

IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       FarbleGetSupportedExtensions) {
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
  std::string actual_value_off =
      EvalJs(contents(), kTitleScript).ExtractString();
  EXPECT_NE(actual_value_off, kSupportedExtensionsMax);

  // Farbling level: balanced (default)
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const auto actual_balanced_value =
      EvalJs(contents(), kTitleScript).ExtractString();
  VerifyBalancedFarblingExtensions(actual_value_off, actual_balanced_value,
                                   GetParam());
}

IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest, FarbleGetExtension) {
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
  std::string actual_value_off =
      EvalJs(contents(), kTitleScript).ExtractString();
  EXPECT_NE(actual_value_off, kExpectedExtensionListMax);

  // Farbling level: balanced (default)
  // WebGL getExtension returns real objects
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const auto actual_balanced_value =
      EvalJs(contents(), kTitleScript).ExtractString();
  VerifyBalancedFarblingExtensions(actual_value_off, actual_balanced_value,
                                   GetParam());
}

IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       GetExtensionWithInvalidName) {
  std::string domain = "a.com";
  GURL url = embedded_test_server()->GetURL(domain, "/getExtension.html");

  // Farbling level: maximum
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_TRUE(ExecJs(contents(), "getExtensionWithInvalidName()"));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(), "null");

  // Farbling level: off
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_TRUE(ExecJs(contents(), "getExtensionWithInvalidName()"));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(), "null");

  // Farbling level: balanced (default)
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_TRUE(ExecJs(contents(), "getExtensionWithInvalidName()"));
  EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(), "null");
}
