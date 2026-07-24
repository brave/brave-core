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
        {});
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
    return HostContentSettingsMapFactory::GetForProfile(
        browser()->GetProfile());
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
  const std::string kGetWebGL1 = "getWebGL1UnmaskedVendorAndRenderer()";

  // Farbling level: off
  // This is tested below in relation with "maximum" and "balanced"
  // farbling.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual_value_off = EvalJs(contents(), kGetWebGL1).ExtractString();

  // Farbling level: maximum
  // pseudo-random data with no relation to original data
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string expected_value_maximum =
      GetExpectedString(TestFarblingLevel::MAXIMUM);
  std::string actual_value_maximum =
      EvalJs(contents(), kGetWebGL1).ExtractString();
  EXPECT_EQ(expected_value_maximum, actual_value_maximum);
  // second time, same as the first (tests that results are consistent for the
  // lifetime of a session, and that the PRNG properly resets itself at the
  // beginning of each calculation)
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  actual_value_maximum = EvalJs(contents(), kGetWebGL1).ExtractString();
  EXPECT_EQ(expected_value_maximum, actual_value_maximum);
  // Check never same as the "off" state.
  EXPECT_NE(actual_value_off, actual_value_maximum);

  // Farbling level: balanced (default)
  // If feature flag was "on", we do farbling otherwise not.
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual_value_balanced =
      EvalJs(contents(), kGetWebGL1).ExtractString();
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
  const std::string kGetWebGL1Extensions = "getWebGL1SupportedExtensions()";
  // Farbling level: maximum
  // WebGL getSupportedExtensions returns abbreviated list
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL1Extensions).ExtractString(),
            kSupportedExtensionsMax);

  // Farbling level: off
  // WebGL getSupportedExtensions is real
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual_value_off =
      EvalJs(contents(), kGetWebGL1Extensions).ExtractString();
  EXPECT_NE(actual_value_off, kSupportedExtensionsMax);

  // Farbling level: balanced (default)
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const auto actual_balanced_value =
      EvalJs(contents(), kGetWebGL1Extensions).ExtractString();
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

// Regression test for https://github.com/brave/brave-browser/issues/57902
//
// Plotly.js Scattergl (via regl) lowercases required WebGL extension names
// before calling getExtension(). Chromium matches case-insensitively; Brave's
// farbling wrapper must do the same or createRegl fails and Plotly shows
// "WebGL is not supported by your browser".
IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       GetExtensionMatchesCaseInsensitivelyLikeRegl) {
  const std::string domain = "a.com";
  const GURL url = embedded_test_server()->GetURL(domain, "/getExtension.html");
  // Plotly Scattergl / splom required extensions, lowercased by regl.
  const std::string kExpectedPlotlyRegl =
      "ANGLE_instanced_arrays:ok,OES_element_index_uint:ok";

  auto run_checks = [&](const char* fingerprinting_label) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(ExecJs(contents(), "getExtensionPlotlyReglStyle()"))
        << fingerprinting_label;
    EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(),
              kExpectedPlotlyRegl)
        << fingerprinting_label;

    ASSERT_TRUE(ExecJs(contents(), "getExtensionLowercaseAllSupported()"))
        << fingerprinting_label;
    EXPECT_EQ(EvalJs(contents(), kTitleScript).ExtractString(), "ok")
        << fingerprinting_label;
  };

  // Shields / fingerprinting disabled (reported in the issue).
  AllowFingerprinting(domain);
  run_checks("farbling off");

  // Default shields (balanced farbling).
  SetFingerprintingDefault(domain);
  run_checks("farbling balanced");
}

IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       FarbleGetSupportedExtensionsWebGL2) {
  std::string domain = "a.com";
  GURL url =
      embedded_test_server()->GetURL(domain, "/getSupportedExtensions.html");
  const std::string kSupportedExtensionsMax = "WEBGL_debug_renderer_info";
  const std::string kGetWebGL2Extensions = "getWebGL2SupportedExtensions()";
  // Farbling level: maximum
  // WebGL2 getSupportedExtensions returns abbreviated list
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL2Extensions).ExtractString(),
            kSupportedExtensionsMax);

  // Farbling level: off
  // WebGL2 getSupportedExtensions is real
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string actual_value_off =
      EvalJs(contents(), kGetWebGL2Extensions).ExtractString();
  ASSERT_FALSE(actual_value_off.empty());
  EXPECT_NE(actual_value_off, kSupportedExtensionsMax);

  // Farbling level: balanced (default)
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const auto actual_balanced_value =
      EvalJs(contents(), kGetWebGL2Extensions).ExtractString();
  VerifyBalancedFarblingExtensions(actual_value_off, actual_balanced_value,
                                   GetParam());
}

// Regression test for https://github.com/brave/brave-browser/issues/57736
IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       WebGL1ExtensionsNotContaminatedByWebGL2) {
  const std::string domain = "a.com";
  const GURL url =
      embedded_test_server()->GetURL(domain, "/getSupportedExtensions.html");

  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  // Baseline: WebGL1 alone should expose ANGLE_instanced_arrays on desktop.
  const std::string baseline =
      EvalJs(contents(), "getWebGL1SupportedExtensions()").ExtractString();
  ASSERT_FALSE(baseline.empty());
  ASSERT_NE(baseline.find("ANGLE_instanced_arrays"), std::string::npos)
      << "Baseline WebGL1 extensions: " << baseline;

  // Fresh document: probe WebGL2 first, then inspect WebGL1.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_FALSE(EvalJs(contents(), "getWebGL2SupportedExtensions()")
                   .ExtractString()
                   .empty());
  const std::string extensions =
      EvalJs(contents(), "getWebGL1SupportedExtensions()").ExtractString();
  ASSERT_FALSE(extensions.empty());

  EXPECT_NE(extensions.find("ANGLE_instanced_arrays"), std::string::npos)
      << "WebGL1 lost ANGLE_instanced_arrays after a WebGL2 probe. "
         "extensions="
      << extensions;
  EXPECT_EQ(extensions.find("EXT_disjoint_timer_query_webgl2"),
            std::string::npos)
      << "WebGL1 exposed a WebGL2-only extension after a WebGL2 probe. "
         "extensions="
      << extensions;
}

// Regression test for https://github.com/brave/brave-browser/issues/57736
IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       WebGL2ExtensionsNotContaminatedByWebGL1) {
  const std::string domain = "a.com";
  const GURL url =
      embedded_test_server()->GetURL(domain, "/getSupportedExtensions.html");

  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  // Baseline: WebGL2 alone should expose a WebGL2-only extension on desktop.
  const std::string baseline =
      EvalJs(contents(), "getWebGL2SupportedExtensions()").ExtractString();
  ASSERT_FALSE(baseline.empty());
  ASSERT_NE(baseline.find("EXT_disjoint_timer_query_webgl2"), std::string::npos)
      << "Baseline WebGL2 extensions: " << baseline;

  // Fresh document: probe WebGL1 first, then inspect WebGL2.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_FALSE(EvalJs(contents(), "getWebGL1SupportedExtensions()")
                   .ExtractString()
                   .empty());
  const std::string extensions =
      EvalJs(contents(), "getWebGL2SupportedExtensions()").ExtractString();
  ASSERT_FALSE(extensions.empty());

  EXPECT_NE(extensions.find("EXT_disjoint_timer_query_webgl2"),
            std::string::npos)
      << "WebGL2 lost EXT_disjoint_timer_query_webgl2 after a WebGL1 probe. "
         "extensions="
      << extensions;
  EXPECT_EQ(extensions.find("ANGLE_instanced_arrays"), std::string::npos)
      << "WebGL2 exposed a WebGL1-only extension after a WebGL1 probe. "
         "extensions="
      << extensions;
}

// BRAVE_WEBCOMPAT_WEBGL and BRAVE_WEBCOMPAT_WEBGL2 exceptions must apply only
// to their respective API versions.
IN_PROC_BROWSER_TEST_P(BraveWebGLExtensionFarblingTest,
                       WebcompatExceptionsAreIndependentPerWebGLVersion) {
  const std::string domain = "a.com";
  const GURL url =
      embedded_test_server()->GetURL(domain, "/getSupportedExtensions.html");
  const std::string kSupportedExtensionsMax = "WEBGL_debug_renderer_info";
  const std::string kGetWebGL1Extensions = "getWebGL1SupportedExtensions()";
  const std::string kGetWebGL2Extensions = "getWebGL2SupportedExtensions()";

  // Real (unfarbled) baselines.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string webgl1_off =
      EvalJs(contents(), kGetWebGL1Extensions).ExtractString();
  const std::string webgl2_off =
      EvalJs(contents(), kGetWebGL2Extensions).ExtractString();
  ASSERT_FALSE(webgl1_off.empty());
  ASSERT_FALSE(webgl2_off.empty());
  ASSERT_NE(webgl1_off, kSupportedExtensionsMax);
  ASSERT_NE(webgl2_off, kSupportedExtensionsMax);

  // Maximum farbling: both versions abbreviated.
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL1Extensions).ExtractString(),
            kSupportedExtensionsMax);
  EXPECT_EQ(EvalJs(contents(), kGetWebGL2Extensions).ExtractString(),
            kSupportedExtensionsMax);

  // Exception for WebGL1 only: WebGL1 unfarbled, WebGL2 still maximum.
  brave_shields::SetWebcompatEnabled(content_settings(),
                                     ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
                                     true, url, nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL1Extensions).ExtractString(),
            webgl1_off);
  EXPECT_EQ(EvalJs(contents(), kGetWebGL2Extensions).ExtractString(),
            kSupportedExtensionsMax);

  // Exception for WebGL2 only: WebGL2 unfarbled, WebGL1 still maximum.
  brave_shields::SetWebcompatEnabled(content_settings(),
                                     ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
                                     false, url, nullptr);
  brave_shields::SetWebcompatEnabled(
      content_settings(), ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2, true,
      url, nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL1Extensions).ExtractString(),
            kSupportedExtensionsMax);
  EXPECT_EQ(EvalJs(contents(), kGetWebGL2Extensions).ExtractString(),
            webgl2_off);
}

// BRAVE_WEBCOMPAT_WEBGL and BRAVE_WEBCOMPAT_WEBGL2 must independently control
// UNMASKED_VENDOR_WEBGL / UNMASKED_RENDERER_WEBGL farbling.
IN_PROC_BROWSER_TEST_P(
    BraveWebGLExtensionFarblingTest,
    DebugInfoWebcompatExceptionsAreIndependentPerWebGLVersion) {
  // Balanced "Brave" farbling only applies when the feature is enabled.
  if (!GetParam()) {
    GTEST_SKIP() << "Requires kWebGLBalancedFingerprintingProtection";
  }

  const std::string domain = "a.com";
  const GURL url = embedded_test_server()->GetURL(domain, "/getParameter.html");
  const std::string kBraveDebugInfo = "Brave,Brave";
  const std::string kGetWebGL1 = "getWebGL1UnmaskedVendorAndRenderer()";
  const std::string kGetWebGL2 = "getWebGL2UnmaskedVendorAndRenderer()";

  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string webgl1_off = EvalJs(contents(), kGetWebGL1).ExtractString();
  const std::string webgl2_off = EvalJs(contents(), kGetWebGL2).ExtractString();
  ASSERT_FALSE(webgl1_off.empty());
  ASSERT_FALSE(webgl2_off.empty());
  ASSERT_NE(webgl1_off, kBraveDebugInfo);
  ASSERT_NE(webgl2_off, kBraveDebugInfo);

  // Default farbling: both versions report "Brave".
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL1).ExtractString(), kBraveDebugInfo);
  EXPECT_EQ(EvalJs(contents(), kGetWebGL2).ExtractString(), kBraveDebugInfo);

  // Exception for WebGL1 only: WebGL1 real, WebGL2 still "Brave".
  brave_shields::SetWebcompatEnabled(content_settings(),
                                     ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
                                     true, url, nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL1).ExtractString(), webgl1_off);
  EXPECT_EQ(EvalJs(contents(), kGetWebGL2).ExtractString(), kBraveDebugInfo);

  // Exception for WebGL2 only: WebGL2 real, WebGL1 still "Brave".
  brave_shields::SetWebcompatEnabled(content_settings(),
                                     ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
                                     false, url, nullptr);
  brave_shields::SetWebcompatEnabled(
      content_settings(), ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2, true,
      url, nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGetWebGL1).ExtractString(), kBraveDebugInfo);
  EXPECT_EQ(EvalJs(contents(), kGetWebGL2).ExtractString(), webgl2_off);
}
