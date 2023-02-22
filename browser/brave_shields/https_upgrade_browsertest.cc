/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/https_only_mode_upgrade_interceptor.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/common/content_client.h"
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
#include "chrome/test/base/in_process_browser_test.h"
#endif

using brave_shields::ControlType;
using net::features::kBraveHttpsByDefault;

namespace {

enum class PageResult { kHttp, kHttps, kInterstitial };

struct TestCase {
  bool init_secure;
  const char* domain;
  ControlType control_type;
  PageResult expected_result;
};

constexpr TestCase kTestCases[] = {
    {false, "insecure1.test", ControlType::ALLOW, PageResult::kHttp},
    {false, "insecure2.test", ControlType::BLOCK_THIRD_PARTY,
     PageResult::kHttp},
    {false, "insecure3.test", ControlType::BLOCK, PageResult::kInterstitial},
    {false, "upgradable1.test", ControlType::ALLOW, PageResult::kHttp},
    {false, "upgradable2.test", ControlType::BLOCK_THIRD_PARTY,
     PageResult::kHttps},
    {false, "upgradable3.test", ControlType::BLOCK, PageResult::kHttps},
    {true, "secure1.test", ControlType::ALLOW, PageResult::kHttps},
    {true, "secure2.test", ControlType::BLOCK_THIRD_PARTY, PageResult::kHttps},
    {true, "secure3.test", ControlType::BLOCK, PageResult::kHttps}};

base::FilePath GetTestDataDir() {
  return base::FilePath(FILE_PATH_LITERAL("net/data/url_request_unittest"));
}

}  // namespace

class HttpsUpgradeBrowserTest : public PlatformBrowserTest {
 public:
  HttpsUpgradeBrowserTest() = default;
  ~HttpsUpgradeBrowserTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(kBraveHttpsByDefault);
    PlatformBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    content::SetBrowserClientForTesting(&client_);
    g_brave_browser_process->https_upgrade_exceptions_service()
        ->SetIsReadyForTesting();
    // By default allow all hosts on HTTPS.
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    // Set up "insecure.test" as a hostname with an SSL error. kHttps upgrades
    // to this host will fail, (or fall back in some cases).
    scoped_refptr<net::X509Certificate> cert(https_server_.GetCertificate());
    net::CertVerifyResult verify_result;
    verify_result.is_issued_by_known_root = false;
    verify_result.verified_cert = cert;
    verify_result.cert_status = net::CERT_STATUS_COMMON_NAME_INVALID;
    for (const std::string& host :
         {"insecure1.test", "insecure2.test", "insecure3.test"}) {
      mock_cert_verifier_.mock_cert_verifier()->AddResultForCertAndHost(
          cert, host, verify_result, net::ERR_CERT_INVALID);
    }

    http_server_.AddDefaultHandlers(GetTestDataDir());
    https_server_.AddDefaultHandlers(GetTestDataDir());
    ASSERT_TRUE(http_server_.Start());
    ASSERT_TRUE(https_server_.Start());

    HttpsOnlyModeUpgradeInterceptor::SetHttpsPortForTesting(
        https_server()->port());
    HttpsOnlyModeUpgradeInterceptor::SetHttpPortForTesting(
        http_server()->port());
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

  void AttemptToNavigateToURL(const GURL& url) {
    content::NavigateToURLBlockUntilNavigationsComplete(Contents(), url, 1,
                                                        true);
  }

  GURL RunTestCaseNavigation(bool shields_enabled,
                             bool global_setting,
                             const TestCase& test_case) {
    SCOPED_TRACE(testing::Message()
                 << "global_setting: " << global_setting << ", "
                 << "test_case.init_secure: " << test_case.init_secure << ", "
                 << "test_case.domain: " << test_case.domain << ", "
                 << "test_case.control_type: " << test_case.control_type);
    GURL initial_url =
        test_case.init_secure
            ? https_server()->GetURL(test_case.domain, "/simple.html")
            : http_server()->GetURL(test_case.domain, "/simple.html");
    brave_shields::SetBraveShieldsEnabled(ContentSettings(), shields_enabled,
                                          initial_url, nullptr);
    brave_shields::SetHttpsUpgradeControlType(
        ContentSettings(), test_case.control_type,
        global_setting ? GURL() : initial_url,
        g_browser_process->local_state());
    AttemptToNavigateToURL(initial_url);
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
  BraveContentBrowserClient client_;
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
      RunTestCaseNavigation(true, global_setting, test_case);
      bool interstitial_showing =
          chrome_browser_interstitials::IsShowingInterstitial(Contents());
      if (test_case.expected_result == PageResult::kInterstitial) {
        EXPECT_TRUE(interstitial_showing);
      } else {
        EXPECT_FALSE(interstitial_showing);
        GURL final_url =
            (test_case.expected_result == PageResult::kHttp ? http_server()
                                                            : https_server())
                ->GetURL(test_case.domain, "/simple.html");
        EXPECT_EQ(final_url, Contents()->GetLastCommittedURL());
      }
    }
  }
}

IN_PROC_BROWSER_TEST_F(HttpsUpgradeBrowserTest, CheckUpgradesWithShieldsDown) {
  for (bool global_setting : {true, false}) {
    for (const TestCase& test_case : kTestCases) {
      const GURL initial_url =
          RunTestCaseNavigation(false, global_setting, test_case);
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
      const GURL initial_url =
          RunTestCaseNavigation(true, global_setting, test_case);
      // Disabled flag means no URLs change and no interstitials shown.
      EXPECT_EQ(initial_url, Contents()->GetLastCommittedURL());
      bool interstitial_showing =
          chrome_browser_interstitials::IsShowingInterstitial(Contents());
      EXPECT_FALSE(interstitial_showing);
    }
  }
}
