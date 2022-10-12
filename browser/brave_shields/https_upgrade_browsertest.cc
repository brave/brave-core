/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/https_upgrade_exceptions_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/interstitials/security_interstitial_page_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/https_only_mode_upgrade_interceptor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"

using blink::features::kHttpsByDefault;
using brave_shields::ControlType;

class HttpsUpgradeBrowserTest : public InProcessBrowserTest {
 public:
  HttpsUpgradeBrowserTest() = default;
  ~HttpsUpgradeBrowserTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(kHttpsByDefault);
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    g_brave_browser_process->https_upgrade_exceptions_service()
        ->SetIsReadyForTesting();
    // By default allow all hosts on HTTPS.
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    // Set up "insecure.test" as a hostname with an SSL error. HTTPS upgrades
    // to this host will fail, (or fall back in some cases).
    scoped_refptr<net::X509Certificate> cert(https_server_.GetCertificate());
    net::CertVerifyResult verify_result;
    verify_result.is_issued_by_known_root = false;
    verify_result.verified_cert = cert;
    verify_result.cert_status = net::CERT_STATUS_COMMON_NAME_INVALID;
    mock_cert_verifier_.mock_cert_verifier()->AddResultForCertAndHost(
        cert, "insecure1.test", verify_result, net::ERR_CERT_INVALID);
    mock_cert_verifier_.mock_cert_verifier()->AddResultForCertAndHost(
        cert, "insecure2.test", verify_result, net::ERR_CERT_INVALID);
    mock_cert_verifier_.mock_cert_verifier()->AddResultForCertAndHost(
        cert, "insecure3.test", verify_result, net::ERR_CERT_INVALID);

    http_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    ASSERT_TRUE(http_server_.Start());
    ASSERT_TRUE(https_server_.Start());

    HttpsOnlyModeUpgradeInterceptor::SetHttpsPortForTesting(
        https_server()->port());
    HttpsOnlyModeUpgradeInterceptor::SetHttpPortForTesting(
        http_server()->port());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    return WaitForLoadStop(Contents());
  }

  content::WebContents* Contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  HostContentSettingsMap* ContentSettings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

 protected:
  net::EmbeddedTestServer* http_server() { return &http_server_; }
  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer http_server_{net::EmbeddedTestServer::TYPE_HTTP};
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
  content::ContentMockCertVerifier mock_cert_verifier_;
};

enum PageResult { HTTP, HTTPS, INTERSTITIAL };

struct TestStruct {
  bool init_secure;
  const char* domain;
  ControlType control_type;
  PageResult expected_result;
};

const TestStruct test_combinations[] = {
    {false, "insecure1.test", ControlType::ALLOW, PageResult::HTTP},
    {false, "insecure2.test", ControlType::BLOCK_THIRD_PARTY, PageResult::HTTP},
    {false, "insecure3.test", ControlType::BLOCK, PageResult::INTERSTITIAL},
    {false, "upgradable1.test", ControlType::ALLOW, PageResult::HTTP},
    {false, "upgradable2.test", ControlType::BLOCK_THIRD_PARTY,
     PageResult::HTTPS},
    {false, "upgradable3.test", ControlType::BLOCK, PageResult::HTTPS},
    {true, "secure1.test", ControlType::ALLOW, PageResult::HTTPS},
    {true, "secure2.test", ControlType::BLOCK_THIRD_PARTY, PageResult::HTTPS},
    {true, "secure3.test", ControlType::BLOCK, PageResult::HTTPS}};

// If the user navigates to an HTTP URL for a site that supports HTTPS, the
// navigation should end up on the HTTPS version of the URL.
IN_PROC_BROWSER_TEST_F(HttpsUpgradeBrowserTest, CheckUpgrades) {
  for (bool global_setting : {true, false}) {
    for (const TestStruct test : test_combinations) {
      GURL initial_url =
          test.init_secure ? https_server()->GetURL(test.domain, "/simple.html")
                           : http_server()->GetURL(test.domain, "/simple.html");
      brave_shields::SetHttpsUpgradeControlType(
          ContentSettings(), test.control_type,
          global_setting ? GURL() : initial_url,
          g_browser_process->local_state());
      base::RunLoop().RunUntilIdle();
      content::TestNavigationObserver nav_observer(Contents(), 1);
      NavigateToURLUntilLoadStop(initial_url);
      nav_observer.Wait();
      bool navigation_succeeded = nav_observer.last_navigation_succeeded();
      bool interstitial_showing =
          chrome_browser_interstitials::IsShowingInterstitial(Contents());
      if (test.expected_result == PageResult::INTERSTITIAL) {
        EXPECT_TRUE(interstitial_showing);
        EXPECT_FALSE(navigation_succeeded);
      } else {
        EXPECT_FALSE(interstitial_showing);
        EXPECT_TRUE(navigation_succeeded);
        GURL final_url;
        if (test.expected_result == PageResult::HTTP) {
          final_url = http_server()->GetURL(test.domain, "/simple.html");
        } else {
          final_url = https_server()->GetURL(test.domain, "/simple.html");
        }
        EXPECT_EQ(final_url, Contents()->GetLastCommittedURL());
      }
    }
  }
}
