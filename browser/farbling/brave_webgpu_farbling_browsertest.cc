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
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "gpu/config/gpu_switches.h"
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
  // WebGPU's navigator.gpu is gated behind [SecureContext], so the test pages
  // must be served from a secure origin. Use an HTTPS server backed by a mock
  // cert verifier (which accepts the test cert for any hostname) so the
  // existing per-domain Shields fingerprinting controls keep working.
  // See navigator_gpu.idl in the upstream.
  BraveWebGPUFarblingBrowserTest()
      : https_server_(net::test_server::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitWithFeatures(
        {
            brave_shields::features::kBraveShowStrictFingerprintingMode,
            webcompat::features::kBraveWebcompatExceptionsService,
        },
        {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(&https_server_);

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_.ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    // Exposes navigator.gpu / a real adapter on platforms where WebGPU isn't
    // enabled by default (e.g. Linux CI).
    command_line->AppendSwitch(switches::kEnableUnsafeWebGPU);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(const std::string& domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        https_server_.GetURL(domain, "/"));
  }

  void BlockFingerprinting(const std::string& domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        https_server_.GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(const std::string& domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        https_server_.GetURL(domain, "/"));
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 protected:
  net::EmbeddedTestServer https_server_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test no scrubbing fixture.
class BraveWebGPUDeveloperFeaturesTest : public BraveWebGPUFarblingBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    BraveWebGPUFarblingBrowserTest::SetUpCommandLine(command_line);
    // When --enable-webgpu-developer-features is set,
    // GPUAdapter::CreateAdapterInfoForAdapter() returns the full, real adapter
    // info and the farbling scrubbing is intentionally bypassed.
    command_line->AppendSwitch(switches::kEnableWebGPUDeveloperFeatures);
  }
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

// Windows doesn't seem to have the WebGPU adapter support.
#if BUILDFLAG(IS_WIN)
#define NON_WIN_TEST(test) DISABLED_##test
#else
#define NON_WIN_TEST(test) test
#endif

// Tests that navigator.gpu.requestAdapter() → adapter.info fields (vendor,
// architecture, device) are emptied under BALANCED (feature on) and MAXIMUM
// farbling, and are returned as-is under OFF and BALANCED (feature off).
IN_PROC_BROWSER_TEST_P(BraveWebGPUAdapterInfoTest,
                       NON_WIN_TEST(FarbleAdapterInfo)) {
  const std::string domain = "a.com";
  const GURL url = https_server_.GetURL(domain, "/adapter-info.html");

  // Farbling level: off — collect the real adapter info as a baseline.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_off =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

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
IN_PROC_BROWSER_TEST_P(BraveWebGPUAdapterInfoTest,
                       NON_WIN_TEST(FarbleDeviceAdapterInfo)) {
  const std::string domain = "a.com";
  const GURL url = https_server_.GetURL(domain, "/device-adapter-info.html");

  // Farbling level: off — collect the real adapter info as a baseline.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_off =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

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

// Tests that adapter.info fields (vendor, architecture, device) are NOT
// scrubbed under MAXIMUM farbling when WebGPU developer features are enabled.
IN_PROC_BROWSER_TEST_F(BraveWebGPUDeveloperFeaturesTest,
                       NON_WIN_TEST(AdapterInfoNotScrubbed)) {
  const std::string domain = "a.com";
  const GURL url = https_server_.GetURL(domain, "/adapter-info.html");

  // Farbling level: off — collect the real adapter info as a baseline.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_off =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

  // Developer features expose real values, so the baseline must not be empty.
  ASSERT_NE(actual_off, kEmptyAdapterInfo);

  // Farbling level: maximum (BlockFingerprinting →
  // BraveFarblingLevel::MAXIMUM). Developer features bypass scrubbing, so the
  // info must match the baseline.
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGPUAdapterInfoScript).ExtractString(),
            actual_off);
}

// Tests that device.adapterInfo fields (vendor, architecture, device) are NOT
// scrubbed under MAXIMUM farbling when WebGPU developer features are enabled.
IN_PROC_BROWSER_TEST_F(BraveWebGPUDeveloperFeaturesTest,
                       NON_WIN_TEST(DeviceAdapterInfoNotScrubbed)) {
  const std::string domain = "a.com";
  const GURL url = https_server_.GetURL(domain, "/device-adapter-info.html");

  // Farbling level: off — collect the real adapter info as a baseline.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const std::string actual_off =
      EvalJs(contents(), kGPUAdapterInfoScript).ExtractString();

  // Developer features expose real values, so the baseline must not be empty.
  ASSERT_NE(actual_off, kEmptyAdapterInfo);

  // Farbling level: maximum (BlockFingerprinting →
  // BraveFarblingLevel::MAXIMUM). Developer features bypass scrubbing, so the
  // info must match the baseline.
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(EvalJs(contents(), kGPUAdapterInfoScript).ExtractString(),
            actual_off);
}
