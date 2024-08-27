/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/https_upgrades_interceptor.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#endif

using brave_shields::ControlType;
using net::features::kBraveHttpsByDefault;

namespace {

enum class PageResult { kHttp, kHttps, kInterstitial };

struct TestCase {
  bool init_secure;
  const char* domain;
  const char* path;
  ControlType control_type;
  bool type_url;
  PageResult expected_result;
};

constexpr char kSimple[] = "/simple.html";
// Nonexistent page results in a 404:
constexpr char kNonexistent[] = "/nonexistent.html";

constexpr TestCase kTestCases[] = {
    {false, "insecure1.com", kSimple, ControlType::ALLOW, false,
     PageResult::kHttp},
    {false, "insecure2.com", kSimple, ControlType::BLOCK_THIRD_PARTY, false,
     PageResult::kHttp},
    {false, "insecure3.com", kSimple, ControlType::BLOCK, false,
     PageResult::kInterstitial},
    {false, "broken1.com", kNonexistent, ControlType::ALLOW, false,
     PageResult::kHttp},
    {false, "broken2.com", kNonexistent, ControlType::BLOCK_THIRD_PARTY, false,
     PageResult::kHttps},
    {false, "broken3.com", kNonexistent, ControlType::BLOCK, false,
     PageResult::kHttps},
    {false, "upgradable1.com", kSimple, ControlType::ALLOW, false,
     PageResult::kHttp},
    {false, "upgradable2.com", kSimple, ControlType::BLOCK_THIRD_PARTY, false,
     PageResult::kHttps},
    {false, "upgradable3.com", kSimple, ControlType::BLOCK, false,
     PageResult::kHttps},
    {false, "upgradable1.com", kSimple, ControlType::ALLOW, true,
     PageResult::kHttp},
    {false, "upgradable2.com", kSimple, ControlType::BLOCK_THIRD_PARTY, true,
     PageResult::kHttps},
    {false, "upgradable3.com", kSimple, ControlType::BLOCK, true,
     PageResult::kHttps},
    {true, "secure1.com", kSimple, ControlType::ALLOW, false,
     PageResult::kHttps},
    {true, "secure2.com", kSimple, ControlType::BLOCK_THIRD_PARTY, false,
     PageResult::kHttps},
    {true, "secure3.com", kSimple, ControlType::BLOCK, false,
     PageResult::kHttps}};

base::FilePath GetTestDataDir() {
  return base::FilePath(FILE_PATH_LITERAL("net/data/url_request_unittest"));
}

}  // namespace

class HttpsUpgradeBrowserTest : public PlatformBrowserTest {
 public:
  HttpsUpgradeBrowserTest() = default;
  ~HttpsUpgradeBrowserTest() override = default;

  void SetUp() override {
    feature_list_.InitWithFeatures({kBraveHttpsByDefault}, {});
    PlatformBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    g_brave_browser_process->https_upgrade_exceptions_service()
        ->SetIsReadyForTesting();
    // By default allow all hosts on HTTPS.
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    // Set up "insecure.com" as a hostname with an SSL error. kHttps upgrades
    // to this host will fail, (or fall back in some cases).
    scoped_refptr<net::X509Certificate> cert(https_server_.GetCertificate());
    net::CertVerifyResult verify_result;
    verify_result.is_issued_by_known_root = false;
    verify_result.verified_cert = cert;
    verify_result.cert_status = net::CERT_STATUS_COMMON_NAME_INVALID;
    for (const std::string& host :
         {"insecure1.com", "insecure2.com", "insecure3.com"}) {
      mock_cert_verifier_.mock_cert_verifier()->AddResultForCertAndHost(
          cert, host, verify_result, net::ERR_CERT_INVALID);
    }

    http_server_.AddDefaultHandlers(GetTestDataDir());
    https_server_.AddDefaultHandlers(GetTestDataDir());
    ASSERT_TRUE(http_server_.Start());
    ASSERT_TRUE(https_server_.Start());

    HttpsUpgradesInterceptor::SetHttpsPortForTesting(https_server()->port());
    HttpsUpgradesInterceptor::SetHttpPortForTesting(http_server()->port());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    PlatformBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void AttemptNavigation(const GURL& url, bool url_typed_with_http_scheme) {
#if BUILDFLAG(IS_ANDROID)
    // Chromium Android does not appear to skip upgrading URLs typed
    // with an http scheme, so we don't need a special setting:
    content::NavigateToURLBlockUntilNavigationsComplete(Contents(), url, 1,
                                                        true);
#else
    NavigateParams params(chrome_test_utils::GetProfile(this), url,
                          ui::PAGE_TRANSITION_TYPED);
    params.url_typed_with_http_scheme = url_typed_with_http_scheme;
    ui_test_utils::NavigateToURL(&params);
#endif
  }

  GURL RunTestCaseNavigation(bool shields_enabled,
                             bool global_setting,
                             const TestCase& test_case,
                             bool type_url) {
    SCOPED_TRACE(testing::Message()
                 << "global_setting: " << global_setting << ", "
                 << "test_case.init_secure: " << test_case.init_secure << ", "
                 << "test_case.domain: " << test_case.domain << ", "
                 << "test_case.control_type: " << test_case.control_type);
    GURL initial_url =
        test_case.init_secure
            ? https_server()->GetURL(test_case.domain, test_case.path)
            : http_server()->GetURL(test_case.domain, test_case.path);
    brave_shields::SetBraveShieldsEnabled(ContentSettings(), shields_enabled,
                                          initial_url, nullptr);
    brave_shields::SetHttpsUpgradeControlType(
        ContentSettings(), test_case.control_type,
        global_setting ? GURL() : initial_url,
        g_browser_process->local_state());
    // Run navigation twice to ensure that the behavior doesn't
    // change after first run.
    for (int i = 0; i < 2; ++i) {
      AttemptNavigation(initial_url, type_url);
    }
    return initial_url;
  }

  content::WebContents* Contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  HostContentSettingsMap* ContentSettings() {
    return HostContentSettingsMapFactory::GetForProfile(
        chrome_test_utils::GetProfile(this));
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer* http_server() { return &http_server_; }
  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  net::EmbeddedTestServer http_server_{net::EmbeddedTestServer::TYPE_HTTP};
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  content::ContentMockCertVerifier mock_cert_verifier_;
};

class HttpsUpgradeBrowserTest_FlagDisabled : public HttpsUpgradeBrowserTest {
 public:
  HttpsUpgradeBrowserTest_FlagDisabled() = default;
  ~HttpsUpgradeBrowserTest_FlagDisabled() override = default;

  void SetUp() override {
    feature_list_.InitAndDisableFeature(kBraveHttpsByDefault);
    PlatformBrowserTest::SetUp();
  }
};

IN_PROC_BROWSER_TEST_F(HttpsUpgradeBrowserTest, CheckUpgrades) {
  for (bool global_setting : {true, false}) {
    for (const TestCase& test_case : kTestCases) {
      RunTestCaseNavigation(true, global_setting, test_case,
                            test_case.type_url);
      bool interstitial_showing =
          chrome_browser_interstitials::IsShowingInterstitial(Contents());
      if (test_case.expected_result == PageResult::kInterstitial) {
        EXPECT_TRUE(interstitial_showing);
      } else {
        EXPECT_FALSE(interstitial_showing);
        GURL final_url =
            (test_case.expected_result == PageResult::kHttp ? http_server()
                                                            : https_server())
                ->GetURL(test_case.domain, test_case.path);
        EXPECT_EQ(final_url, Contents()->GetLastCommittedURL());
      }
    }
  }
}

IN_PROC_BROWSER_TEST_F(HttpsUpgradeBrowserTest, CheckUpgradesWithShieldsDown) {
  for (bool global_setting : {true, false}) {
    for (const TestCase& test_case : kTestCases) {
      const GURL initial_url = RunTestCaseNavigation(
          false, global_setting, test_case, test_case.type_url);
      // Shields down means no URLs change and no interstitials shown.
      EXPECT_EQ(initial_url, Contents()->GetLastCommittedURL());
      bool interstitial_showing =
          chrome_browser_interstitials::IsShowingInterstitial(Contents());
      EXPECT_FALSE(interstitial_showing);
    }
  }
}

IN_PROC_BROWSER_TEST_F(HttpsUpgradeBrowserTest_FlagDisabled, CheckUpgrades) {
  for (bool global_setting : {true, false}) {
    for (const TestCase& test_case : kTestCases) {
      const GURL initial_url = RunTestCaseNavigation(
          true, global_setting, test_case, test_case.type_url);
      // Disabled flag means no URLs change and no interstitials shown.
      EXPECT_EQ(initial_url, Contents()->GetLastCommittedURL());
      bool interstitial_showing =
          chrome_browser_interstitials::IsShowingInterstitial(Contents());
      EXPECT_FALSE(interstitial_showing);
    }
  }
}

IN_PROC_BROWSER_TEST_F(HttpsUpgradeBrowserTest, IsolateSettings) {
  // Test host URLs.
  GURL host1("https://example1.com");
  GURL host2("https://example2.com");

  // Test profiles
  Profile* normal_profile = chrome_test_utils::GetProfile(this);
  Profile* incognito_profile = normal_profile->GetOffTheRecordProfile(
      Profile::OTRProfileID::PrimaryID(), /*create_if_needed=*/true);

  auto* normal_map =
      HostContentSettingsMapFactory::GetForProfile(normal_profile);
  auto* incognito_map =
      HostContentSettingsMapFactory::GetForProfile(incognito_profile);

  // Disable upgrades for a site.
  brave_shields::SetHttpsUpgradeControlType(normal_map, ControlType::ALLOW,
                                            host1);
  brave_shields::SetHttpsUpgradeControlType(incognito_map, ControlType::ALLOW,
                                            host2);

  // Disabled upgrade per-site in normal windows should not apply to incognito
  // windows, nor vice versa.
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetHttpsUpgradeControlType(normal_map, host1));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetHttpsUpgradeControlType(incognito_map, host1));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetHttpsUpgradeControlType(normal_map, host2));
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetHttpsUpgradeControlType(incognito_map, host2));

  // Set strict per-site settings.
  brave_shields::SetHttpsUpgradeControlType(normal_map, ControlType::BLOCK,
                                            host1);
  brave_shields::SetHttpsUpgradeControlType(incognito_map, ControlType::BLOCK,
                                            host2);

  // A strict per-site setting in normal windows does apply to incognito
  // windows, but not vice versa.
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetHttpsUpgradeControlType(normal_map, host1));
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetHttpsUpgradeControlType(incognito_map, host1));
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetHttpsUpgradeControlType(incognito_map, host2));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetHttpsUpgradeControlType(normal_map, host2));

  // Set global setting to strict.
  brave_shields::SetHttpsUpgradeControlType(normal_map, ControlType::BLOCK,
                                            GURL());

  // Strict global upgrades should apply to both normal and incognito windows.
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetHttpsUpgradeControlType(normal_map, GURL()));
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetHttpsUpgradeControlType(incognito_map, GURL()));

  // Set global setting to disabled.
  brave_shields::SetHttpsUpgradeControlType(normal_map, ControlType::ALLOW,
                                            GURL());

  // Disabled global upgrades should apply to normal windows but not incognito.
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetHttpsUpgradeControlType(normal_map, GURL()));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetHttpsUpgradeControlType(incognito_map, GURL()));
}
