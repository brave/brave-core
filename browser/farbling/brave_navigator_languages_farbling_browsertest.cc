/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/brave_farbling_service_factory.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/language/core/browser/pref_names.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;
using brave_shields::features::kBraveReduceLanguage;
using brave_shields::features::kBraveShowStrictFingerprintingMode;
using content::TitleWatcher;

namespace {
constexpr char kNavigatorLanguagesScript[] = "navigator.languages.toString()";
const uint64_t kTestingSessionToken = 12345;
}  // namespace

class BraveNavigatorLanguagesFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveNavigatorLanguagesFarblingBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitWithFeatures(
        {kBraveReduceLanguage, kBraveShowStrictFingerprintingMode}, {});
  }

  BraveNavigatorLanguagesFarblingBrowserTest(
      const BraveNavigatorLanguagesFarblingBrowserTest&) = delete;
  BraveNavigatorLanguagesFarblingBrowserTest& operator=(
      const BraveNavigatorLanguagesFarblingBrowserTest&) = delete;

  ~BraveNavigatorLanguagesFarblingBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    brave::BraveFarblingServiceFactory::GetForProfile(browser()->profile())
        ->set_session_tokens_for_testing(kTestingSessionToken);

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveNavigatorLanguagesFarblingBrowserTest::MonitorHTTPRequest,
        base::Unretained(this)));
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        https_server_.GetURL(domain, "/"));
  }

  void BlockFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        https_server_.GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        https_server_.GetURL(domain, "/"));
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void SetAcceptLanguages(const std::string& accept_languages) {
    content::BrowserContext* context =
        static_cast<content::BrowserContext*>(browser()->profile());
    PrefService* prefs = user_prefs::UserPrefs::Get(context);
    prefs->Set(language::prefs::kSelectedLanguages,
               base::Value(accept_languages));
  }

  void MonitorHTTPRequest(const net::test_server::HttpRequest& request) {
    if (request.relative_url.find("/reduce-language/") == std::string::npos) {
      return;
    }
    if (expected_http_accept_language_.empty()) {
      return;
    }
    EXPECT_EQ(request.headers.at("accept-language"),
              expected_http_accept_language_);
  }

  void SetExpectedHTTPAcceptLanguage(
      const std::string& expected_http_accept_language) {
    expected_http_accept_language_ = expected_http_accept_language;
  }

 private:
  std::string expected_http_accept_language_;
};

// Tests results of farbling known values
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleLanguages) {
  std::string domain1 = "b.test";
  std::string domain2 = "d.test";
  GURL url1 = https_server_.GetURL(domain1, "/simple.html");
  GURL url2 = https_server_.GetURL(domain2, "/simple.html");
  // Farbling level: off
  AllowFingerprinting(domain1);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url1));
  std::string testing_languages = "en-US,en,es,la";
  SetAcceptLanguages(testing_languages);
  EXPECT_EQ(testing_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
  AllowFingerprinting(domain2);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url2));
  EXPECT_EQ(testing_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));

  // Farbling level: default
  SetFingerprintingDefault(domain1);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url1));
  std::string standard_languages = "en-US";
  EXPECT_EQ(standard_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
  SetFingerprintingDefault(domain2);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url2));
  EXPECT_EQ(standard_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));

  // Farbling level: maximum
  BlockFingerprinting(domain1);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url1));
  std::string strict_languages = "en-US,en";
  EXPECT_EQ(strict_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
  BlockFingerprinting(domain2);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url2));
  EXPECT_EQ(strict_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
}

// Tests that web workers inherit the farbled navigator.languages
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleLanguagesWebWorker) {
  std::u16string expected_title(u"pass");
  std::string domain = "b.test";
  GURL url = https_server_.GetURL(domain, "/navigator/workers-languages.html");

  // Farbling level: off
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  TitleWatcher watcher1(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher1.WaitAndGetTitle());

  // Farbling level: default
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  TitleWatcher watcher2(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher2.WaitAndGetTitle());

  // Farbling level: maximum
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  TitleWatcher watcher3(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher3.WaitAndGetTitle());
}

// Tests that service workers inherit the farbled navigator.languages
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleLanguagesServiceWorker) {
  std::u16string expected_title(u"pass");
  std::string domain = "b.test";
  GURL url =
      https_server_.GetURL(domain, "/navigator/service-workers-languages.html");
  // Farbling level: default
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  TitleWatcher watcher2(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher2.WaitAndGetTitle());
}

// Tests results of farbling HTTP Accept-Language header
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleHTTPAcceptLanguage) {
  std::string domain_b = "b.test";
  std::string domain_d = "d.test";
  std::string domain_x = "www.ulta.com";
  std::string domain_y = "aeroplan.rewardops.com";
  std::string domain_z = "login.live.com";
  GURL url_b = https_server_.GetURL(
      domain_b, "/reduce-language/page-with-subresources.html");
  GURL url_d = https_server_.GetURL(
      domain_d, "/reduce-language/page-with-subresources.html");
  GURL url_x = https_server_.GetURL(
      domain_x, "/reduce-language/page-with-subresources.html");
  GURL url_y = https_server_.GetURL(
      domain_y, "/reduce-language/page-with-subresources.html");
  GURL url_z = https_server_.GetURL(
      domain_z, "/reduce-language/page-with-subresources.html");
  SetAcceptLanguages("la,es,en");

  // Farbling level: off
  // HTTP Accept-Language header should not be farbled.
  AllowFingerprinting(domain_b);
  SetExpectedHTTPAcceptLanguage("la,es;q=0.9,en;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  AllowFingerprinting(domain_d);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: default
  // HTTP Accept-Language header should be farbled by domain.
  SetFingerprintingDefault(domain_b);
  SetExpectedHTTPAcceptLanguage("la;q=0.7");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  SetExpectedHTTPAcceptLanguage("la;q=0.8");
  SetFingerprintingDefault(domain_d);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: maximum
  // HTTP Accept-Language header should be farbled but the same across domains.
  BlockFingerprinting(domain_b);
  SetExpectedHTTPAcceptLanguage("en-US,en;q=0.9");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  BlockFingerprinting(domain_d);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Test with subdivided language code as the primary language.
  SetAcceptLanguages("zh-HK,zh,la");

  // Farbling level: off
  // HTTP Accept-Language header should not be farbled.
  AllowFingerprinting(domain_b);
  SetExpectedHTTPAcceptLanguage("zh-HK,zh;q=0.9,la;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  AllowFingerprinting(domain_d);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: default
  // HTTP Accept-Language header should be farbled by domain.
  SetFingerprintingDefault(domain_b);
  SetExpectedHTTPAcceptLanguage("zh-HK,zh;q=0.7");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  SetExpectedHTTPAcceptLanguage("zh-HK,zh;q=0.8");
  SetFingerprintingDefault(domain_d);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: maximum
  // HTTP Accept-Language header should be farbled but the same across domains.
  BlockFingerprinting(domain_b);
  SetExpectedHTTPAcceptLanguage("en-US,en;q=0.9");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  BlockFingerprinting(domain_d);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: maximum but domain is on exceptions list
  // HTTP Accept-Language header should not be farbled.
  BlockFingerprinting(domain_x);
  SetExpectedHTTPAcceptLanguage("zh-HK,zh;q=0.9,la;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_x));
  BlockFingerprinting(domain_y);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_y));
  BlockFingerprinting(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
}

// Tests results of farbling HTTP Accept-Language header
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleHTTPAcceptLanguageFromServiceWorker) {
  std::string domain_b = "b.test";
  GURL url_b_sw = https_server_.GetURL(
      domain_b, "/reduce-language/service-workers-accept-language.html");

  // Farbling level: maximum
  // HTTP Accept-Language header should be farbled by the same across domains,
  // even if fetch originated from a service worker.
  SetFingerprintingDefault(domain_b);
  SetAcceptLanguages("zh-HK,zh,la");
  SetExpectedHTTPAcceptLanguage("zh-HK,zh;q=0.7");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b_sw));
  std::u16string expected_title(u"LOADED");
  TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}
