/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/language/core/browser/pref_names.h"
#include "components/network_session_configurator/common/network_switches.h"
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

// Returns the zero-based index of `header_name` among the request headers in a
// raw header block (as exposed via
// `net::test_server::HttpRequest::all_headers`). The block starts with the
// request line (e.g. "GET / HTTP/1.1"), which is ignored, followed by one
// header per line. Returns std::nullopt if the header is not present.
std::optional<size_t> GetHeaderPosition(const std::string& all_headers,
                                        std::string_view header_name) {
  std::vector<std::string> lines = base::SplitString(
      all_headers, "\r\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  // Skip the request line at index 0; headers start at index 1.
  for (size_t i = 1; i < lines.size(); ++i) {
    size_t colon_pos = lines[i].find(':');
    if (colon_pos == std::string::npos) {
      continue;
    }
    std::string_view name = base::TrimWhitespaceASCII(
        std::string_view(lines[i]).substr(0, colon_pos), base::TRIM_ALL);
    if (base::EqualsCaseInsensitiveASCII(name, header_name)) {
      return i - 1;
    }
  }
  return std::nullopt;
}
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

  // When true, the request monitor records the position of the Accept-Language
  // header from the main document request into `accept_language_position_`.
  bool capture_accept_language_position_ = false;
  // Position of the Accept-Language header in the most recently observed main
  // document request (see `capture_accept_language_position_`).
  std::optional<size_t> accept_language_position_;

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
    // Extract host from the request to look up domain-specific expected value.
    // This avoids race conditions where subresource requests from one page
    // arrive after expected value has been changed for the next page.
    auto host_it = request.headers.find("Host");
    if (host_it == request.headers.end()) {
      return;
    }
    // Extract domain from Host header (format: "domain:port")
    std::string host = host_it->second;
    size_t port_pos = host.find(':');
    std::string domain =
        (port_pos != std::string::npos) ? host.substr(0, port_pos) : host;

    // Capture the position of the Accept-Language header within the main
    // document request so the test can assert it does not vary with the
    // farbling state. See https://github.com/brave/brave-browser/issues/55271.
    if (capture_accept_language_position_ &&
        request.relative_url.find("page-with-subresources.html") !=
            std::string::npos) {
      accept_language_position_ =
          GetHeaderPosition(request.all_headers, "accept-language");
    }

    auto expected_it = expected_http_accept_language_by_domain_.find(domain);
    if (expected_it == expected_http_accept_language_by_domain_.end()) {
      return;
    }
    EXPECT_EQ(request.headers.at("accept-language"), expected_it->second);
  }

  void SetExpectedHTTPAcceptLanguage(
      const std::string& domain,
      const std::string& expected_http_accept_language) {
    expected_http_accept_language_by_domain_[domain] =
        expected_http_accept_language;
  }

 private:
  // Map from domain to expected HTTP Accept-Language header value.
  // Using a map ensures that requests from different domains are validated
  // against their correct expected values, even if requests arrive
  // out-of-order due to asynchronous network timing.
  std::map<std::string, std::string> expected_http_accept_language_by_domain_;
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
  AllowFingerprinting(domain_d);
  SetExpectedHTTPAcceptLanguage(domain_b, "la,es;q=0.9,en;q=0.8");
  SetExpectedHTTPAcceptLanguage(domain_d, "la,es;q=0.9,en;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: default
  // HTTP Accept-Language header should be farbled by domain.
  SetFingerprintingDefault(domain_b);
  SetFingerprintingDefault(domain_d);
  SetExpectedHTTPAcceptLanguage(domain_b, "la;q=0.8");
  SetExpectedHTTPAcceptLanguage(domain_d, "la;q=0.5");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: maximum
  // HTTP Accept-Language header should be farbled but the same across domains.
  BlockFingerprinting(domain_b);
  BlockFingerprinting(domain_d);
  SetExpectedHTTPAcceptLanguage(domain_b, "en-US,en;q=0.9");
  SetExpectedHTTPAcceptLanguage(domain_d, "en-US,en;q=0.9");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Test with subdivided language code as the primary language.
  SetAcceptLanguages("zh-HK,zh,la");

  // Farbling level: off
  // HTTP Accept-Language header should not be farbled.
  AllowFingerprinting(domain_b);
  AllowFingerprinting(domain_d);
  SetExpectedHTTPAcceptLanguage(domain_b, "zh-HK,zh;q=0.9,la;q=0.8");
  SetExpectedHTTPAcceptLanguage(domain_d, "zh-HK,zh;q=0.9,la;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: default
  // HTTP Accept-Language header should be farbled by domain.
  SetFingerprintingDefault(domain_b);
  SetFingerprintingDefault(domain_d);
  SetExpectedHTTPAcceptLanguage(domain_b, "zh-HK,zh;q=0.8");
  SetExpectedHTTPAcceptLanguage(domain_d, "zh-HK,zh;q=0.5");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: maximum
  // HTTP Accept-Language header should be farbled but the same across domains.
  BlockFingerprinting(domain_b);
  BlockFingerprinting(domain_d);
  SetExpectedHTTPAcceptLanguage(domain_b, "en-US,en;q=0.9");
  SetExpectedHTTPAcceptLanguage(domain_d, "en-US,en;q=0.9");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_d));

  // Farbling level: maximum but domain is on exceptions list
  // HTTP Accept-Language header should not be farbled.
  BlockFingerprinting(domain_x);
  BlockFingerprinting(domain_y);
  BlockFingerprinting(domain_z);
  SetExpectedHTTPAcceptLanguage(domain_x, "zh-HK,zh;q=0.9,la;q=0.8");
  SetExpectedHTTPAcceptLanguage(domain_y, "zh-HK,zh;q=0.9,la;q=0.8");
  SetExpectedHTTPAcceptLanguage(domain_z, "zh-HK,zh;q=0.9,la;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_x));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_y));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
}

// Tests that the position of the HTTP Accept-Language header within the request
// does not change with the farbling level. Otherwise the header's position
// would leak whether farbling is enabled.
// See https://github.com/brave/brave-browser/issues/55271.
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleHTTPAcceptLanguageHeaderPosition) {
  capture_accept_language_position_ = true;
  std::string domain = "b.test";
  // Each navigation uses a unique query string so the request is not served
  // from cache. Otherwise repeated navigations to the same URL would trigger a
  // conditional revalidation that adds cache-related headers (Cache-Control,
  // If-None-Match) and shifts the Accept-Language position independently of the
  // farbling state.
  GURL url_off = https_server_.GetURL(
      domain, "/reduce-language/page-with-subresources.html?off");
  GURL url_default = https_server_.GetURL(
      domain, "/reduce-language/page-with-subresources.html?default");
  GURL url_max = https_server_.GetURL(
      domain, "/reduce-language/page-with-subresources.html?max");
  SetAcceptLanguages("la,es,en");

  // Farbling level: off. This is the baseline position to compare against.
  AllowFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_off));
  std::optional<size_t> position_when_off = accept_language_position_;
  ASSERT_TRUE(position_when_off.has_value());

  // Farbling level: default. Accept-Language is farbled but its position must
  // match the farbling-off position.
  SetFingerprintingDefault(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_default));
  EXPECT_EQ(accept_language_position_, position_when_off) << "farbling=default";

  // Farbling level: maximum. Accept-Language is farbled but its position must
  // match the farbling-off position.
  BlockFingerprinting(domain);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_max));
  EXPECT_EQ(accept_language_position_, position_when_off) << "farbling=max";
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
  SetExpectedHTTPAcceptLanguage(domain_b, "zh-HK,zh;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b_sw));
  std::u16string expected_title(u"LOADED");
  TitleWatcher watcher(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}
