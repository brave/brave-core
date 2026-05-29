/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

using brave_shields::ControlType;

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "webgpu";

// Script to await the Promise set by the test HTML pages.
constexpr char kGPUAdapterInfoScript[] =
    "(async () => await window.gpuAdapterInfoPromise)()";

// Expected result when all three adapter info fields (vendor, architecture,
// device) are farbled to empty strings.
constexpr char kEmptyAdapterInfo[] = ",,";

}  // namespace

class BraveWebGPUFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveWebGPUFarblingBrowserTest() {
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

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(const std::string& domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        embedded_test_server()->GetURL(domain, "/"));
  }

  void BlockFingerprinting(const std::string& domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        embedded_test_server()->GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(const std::string& domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        embedded_test_server()->GetURL(domain, "/"));
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Parameterized over whether kWebGLBalancedFingerprintingProtection is enabled,
// which controls whether BALANCED farbling level scrubs WebGPU adapter info.
class BraveWebGPUAdapterInfoTest : public BraveWebGPUFarblingBrowserTest,
                                   public testing::WithParamInterface<bool> {
 public:
  BraveWebGPUAdapterInfoTest() {
    if (GetParam()) {
      gpu_feature_list_.InitWithFeatures(
          {blink::features::kWebGLBalancedFingerprintingProtection}, {});
    } else {
      gpu_feature_list_.InitWithFeatures(
          {}, {blink::features::kWebGLBalancedFingerprintingProtection});
    }
  }

 private:
  base::test::ScopedFeatureList gpu_feature_list_;
};

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveWebGPUAdapterInfoTest,
    testing::Bool(),
    [](const testing::TestParamInfo<bool>& info) {
      return info.param ? "WebGLBalancedFingerprintingProtection_Enabled"
                        : "WebGLBalancedFingerprintingProtection_Disabled";
    });

// Tests that navigator.gpu.requestAdapter() → adapter.info fields (vendor,
// architecture, device) are emptied under BALANCED (feature on) and MAXIMUM
// farbling, and are returned as-is under OFF and BALANCED (feature off).
IN_PROC_BROWSER_TEST_P(BraveWebGPUAdapterInfoTest, FarbleAdapterInfo) {
  const std::string domain = "a.com";
  const GURL url = embedded_test_server()->GetURL(domain, "/adapter-info.html");

  // Farbling level: off — collect the real adapter info as a baseline.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_off =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

  if (actual_off == "no-gpu" || actual_off == "no-adapter") {
    GTEST_SKIP() << "WebGPU adapter not available in this environment";
  }

  // Farbling level: maximum (BlockFingerprinting → BraveFarblingLevel::MAXIMUM)
  // All three adapter info fields must be empty regardless of the feature flag.
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGPUAdapterInfoScript).ExtractString(),
            kEmptyAdapterInfo);

  // Farbling level: balanced (SetFingerprintingDefault →
  // BraveFarblingLevel::BALANCED). Behaviour depends on the feature flag.
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_balanced =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

  if (GetParam()) {
    // kWebGLBalancedFingerprintingProtection enabled: fields must be empty.
    EXPECT_EQ(actual_balanced, kEmptyAdapterInfo);
  } else {
    // kWebGLBalancedFingerprintingProtection disabled: fields must be real.
    EXPECT_EQ(actual_balanced, actual_off);
  }
}

// Tests that after adapter.requestDevice() the device.adapterInfo fields
// (vendor, architecture, device) are emptied under BALANCED (feature on) and
// MAXIMUM farbling, and are returned as-is under OFF and BALANCED (feature
// off).
IN_PROC_BROWSER_TEST_P(BraveWebGPUAdapterInfoTest, FarbleDeviceAdapterInfo) {
  const std::string domain = "a.com";
  const GURL url =
      embedded_test_server()->GetURL(domain, "/device-adapter-info.html");

  // Farbling level: off — collect the real adapter info as a baseline.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_off =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

  if (actual_off == "no-gpu" || actual_off == "no-adapter" ||
      actual_off == "no-device") {
    GTEST_SKIP() << "WebGPU device not available in this environment";
  }

  // Farbling level: maximum (BlockFingerprinting → BraveFarblingLevel::MAXIMUM)
  // All three adapter info fields must be empty regardless of the feature flag.
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGPUAdapterInfoScript).ExtractString(),
            kEmptyAdapterInfo);

  // Farbling level: balanced (SetFingerprintingDefault →
  // BraveFarblingLevel::BALANCED). Behaviour depends on the feature flag.
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_balanced =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

  if (GetParam()) {
    // kWebGLBalancedFingerprintingProtection enabled: fields must be empty.
    EXPECT_EQ(actual_balanced, kEmptyAdapterInfo);
  } else {
    // kWebGLBalancedFingerprintingProtection disabled: fields must be real.
    EXPECT_EQ(actual_balanced, actual_off);
  }
}
