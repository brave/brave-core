/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/functional/function_ref.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/thread_test_helper.h"
#include "base/version.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
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
#include "components/embedder_support/user_agent_utils.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"

using brave_shields::ControlType;
using content::TitleWatcher;

namespace {

constexpr char kUserAgentScript[] = "navigator.userAgent";

constexpr char kBrandScript[] =
    "navigator.userAgentData.brands[0].brand + '|' + "
    "navigator.userAgentData.brands[1].brand + '|' + "
    "navigator.userAgentData.brands[2].brand";

constexpr char kGetHighEntropyValuesScript[] = R"(
  navigator.userAgentData.getHighEntropyValues(
      ["fullVersionList", "uaFullVersion"]).then(
          (values) => {return values;})
)";

void CheckUserAgentMetadataVersionsList(
    const base::Value::List& versions_list,
    const std::string& expected_version,
    base::FunctionRef<void(const std::string&)> check_greased_version) {
  // Expect 3 items in the list: Brave, Chromium, and greased.
  EXPECT_EQ(3UL, versions_list.size());

  bool has_brave_brand = false;
  bool has_chromium_brand = false;
  for (auto& brand_version : versions_list) {
    const std::string* brand = brand_version.GetDict().FindString("brand");
    ASSERT_NE(nullptr, brand);
    const std::string* version = brand_version.GetDict().FindString("version");
    ASSERT_NE(nullptr, version);
    if (*brand == "Brave") {
      has_brave_brand = true;
      EXPECT_EQ(expected_version, *version);
    } else if (*brand == "Chromium") {
      has_chromium_brand = true;
      EXPECT_EQ(expected_version, *version);
    } else {
      check_greased_version(*version);
    }
  }
  EXPECT_TRUE(has_brave_brand);
  EXPECT_TRUE(has_chromium_brand);
}

}  // namespace

class BraveNavigatorUserAgentFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveNavigatorUserAgentFarblingBrowserTest() {
    feature_list_.InitAndEnableFeature(
        brave_shields::features::kBraveShowStrictFingerprintingMode);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    content::SetupCrossSiteRedirector(https_server_.get());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_->ServeFilesFromDirectory(test_data_dir);
    https_server_->RegisterRequestMonitor(base::BindRepeating(
        &BraveNavigatorUserAgentFarblingBrowserTest::MonitorHTTPRequest,
        base::Unretained(this)));
    user_agents_.clear();

    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void MonitorHTTPRequest(const net::test_server::HttpRequest& request) {
    user_agents_.push_back(request.headers.at("user-agent"));
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  std::string last_requested_http_user_agent() {
    if (user_agents_.empty()) {
      return "";
    }
    return user_agents_[user_agents_.size() - 1];
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        https_server()->GetURL(domain, "/"));
  }

  void BlockFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        https_server()->GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        https_server()->GetURL(domain, "/"));
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::vector<std::string> user_agents_;
  base::test::ScopedFeatureList feature_list_;
};

// Tests results of farbling user agent
IN_PROC_BROWSER_TEST_F(BraveNavigatorUserAgentFarblingBrowserTest,
                       FarbleNavigatorUserAgent) {
  std::u16string expected_title(u"pass");
  std::string domain_b = "b.com";
  std::string domain_z = "z.com";
  GURL url_b = https_server()->GetURL(domain_b, "/simple.html");
  GURL url_z = https_server()->GetURL(domain_z, "/simple.html");
  // get real navigator.userAgent
  std::string unfarbled_ua = embedder_support::GetUserAgent();
  // Farbling level: off
  AllowFingerprinting(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  // HTTP User-Agent header we just sent in that request should be the same as
  // the unfarbled user agent
  EXPECT_EQ(last_requested_http_user_agent(), unfarbled_ua);
  auto off_ua_b = EvalJs(contents(), kUserAgentScript);
  // user agent should be the same as the unfarbled user agent
  EXPECT_EQ(unfarbled_ua, off_ua_b);
  AllowFingerprinting(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  // HTTP User-Agent header we just sent in that request should be the same as
  // the unfarbled user agent
  EXPECT_EQ(last_requested_http_user_agent(), unfarbled_ua);
  auto off_ua_z = EvalJs(contents(), kUserAgentScript);
  // user agent should be the same on every domain if farbling is off
  EXPECT_EQ(unfarbled_ua, off_ua_z);

  // Farbling level: default
  // navigator.userAgent may be farbled, but the farbling is not
  // domain-specific
  SetFingerprintingDefault(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  std::string default_ua_b =
      EvalJs(contents(), kUserAgentScript).ExtractString();
  SetFingerprintingDefault(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  std::string default_ua_z =
      EvalJs(contents(), kUserAgentScript).ExtractString();
  // user agent should be the same on every domain if farbling is default
  EXPECT_EQ(default_ua_b, default_ua_z);

  // Farbling level: maximum
  // navigator.userAgent should be the possibly-farbled string from the default
  // farbling level, further suffixed by a pseudo-random number of spaces based
  // on domain and session key
  BlockFingerprinting(domain_b);
  // test known values
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  auto max_ua_b = EvalJs(contents(), kUserAgentScript);
  EXPECT_EQ(default_ua_b + "   ", max_ua_b);
  BlockFingerprinting(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  auto max_ua_z = EvalJs(contents(), kUserAgentScript);
  EXPECT_EQ(default_ua_z + " ", max_ua_z);

  // test that web workers also inherit the farbled user agent
  // (farbling level is still maximum)
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/workers-useragent.html")));
  // HTTP User-Agent header we just sent in that request should be the same as
  // the unfarbled user agent
  EXPECT_EQ(last_requested_http_user_agent(), unfarbled_ua);
  TitleWatcher watcher3(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher3.WaitAndGetTitle());

  // test that service workers also inherit the farbled user agent
  // (farbling level is still maximum)
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL(
                     domain_b, "/navigator/service-workers-useragent.html")));
  // HTTP User-Agent header we just sent in that request should be the same as
  // the unfarbled user agent
  EXPECT_EQ(last_requested_http_user_agent(), unfarbled_ua);
  TitleWatcher watcher4(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher4.WaitAndGetTitle());

  // Farbling level: off
  // verify that user agent is reset properly after having been farbled
  AllowFingerprinting(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  EXPECT_EQ(last_requested_http_user_agent(), unfarbled_ua);
  auto off_ua_b2 = EvalJs(contents(), kUserAgentScript);
  EXPECT_EQ(off_ua_b.ExtractString(), off_ua_b2);
}

// Tests results of farbling user agent in iframes
IN_PROC_BROWSER_TEST_F(BraveNavigatorUserAgentFarblingBrowserTest,
                       FarbleNavigatorUserAgentIframe) {
  std::u16string expected_title(u"pass");
  std::string domain_b = "b.com";
  GURL url_b = https_server()->GetURL(domain_b, "/simple.html");
  BlockFingerprinting(domain_b);

  // test that local iframes inherit the farbled user agent
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-local-iframe.html")));
  TitleWatcher watcher1(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher1.WaitAndGetTitle());

  // test that remote iframes inherit the farbled user agent
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-remote-iframe.html")));
  TitleWatcher watcher2(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher2.WaitAndGetTitle());

  // test that dynamic iframes inherit the farbled user agent
  // 7 variations based on https://arkenfox.github.io/TZP/tzp.html
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-dynamic-iframe-1.html")));
  TitleWatcher dynamic_iframe_1_watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, dynamic_iframe_1_watcher.WaitAndGetTitle());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-dynamic-iframe-2.html")));
  TitleWatcher dynamic_iframe_2_watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, dynamic_iframe_2_watcher.WaitAndGetTitle());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-dynamic-iframe-3.html")));
  TitleWatcher dynamic_iframe_3_watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, dynamic_iframe_3_watcher.WaitAndGetTitle());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-dynamic-iframe-4.html")));
  TitleWatcher dynamic_iframe_4_watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, dynamic_iframe_4_watcher.WaitAndGetTitle());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-dynamic-iframe-5.html")));
  TitleWatcher dynamic_iframe_5_watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, dynamic_iframe_5_watcher.WaitAndGetTitle());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-dynamic-iframe-6.html")));
  TitleWatcher dynamic_iframe_6_watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, dynamic_iframe_6_watcher.WaitAndGetTitle());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(),
      https_server()->GetURL(domain_b, "/navigator/ua-dynamic-iframe-7.html")));
  TitleWatcher dynamic_iframe_7_watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, dynamic_iframe_7_watcher.WaitAndGetTitle());
}

// Tests results of farbling user agent metadata
IN_PROC_BROWSER_TEST_F(BraveNavigatorUserAgentFarblingBrowserTest,
                       FarbleNavigatorUserAgentModel) {
  GURL url_b = https_server()->GetURL("b.com", "/navigator/useragentdata.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));
  std::u16string expected_title(u"pass");
  TitleWatcher watcher(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher.WaitAndGetTitle());
}

// Tests results of user agent metadata brands
IN_PROC_BROWSER_TEST_F(BraveNavigatorUserAgentFarblingBrowserTest,
                       BraveIsInNavigatorUserAgentBrandList) {
  GURL url = https_server()->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string brands = EvalJs(contents(), kBrandScript).ExtractString();
  EXPECT_NE(std::string::npos, brands.find("Brave"));
  EXPECT_NE(std::string::npos, brands.find("Chromium"));
}

// Tests that user agent metadata versions are as expected.
IN_PROC_BROWSER_TEST_F(BraveNavigatorUserAgentFarblingBrowserTest,
                       CheckUserAgentMetadataVersions) {
  GURL url = https_server()->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  const content::EvalJsResult result =
      EvalJs(contents(), kGetHighEntropyValuesScript);
  EXPECT_TRUE(result.error.empty());
  const base::Value::Dict* values = result.value.GetIfDict();
  ASSERT_NE(nullptr, values);

  // Check brands versions
  const base::Value::List* brands_list = values->FindList("brands");
  ASSERT_NE(nullptr, brands_list);

  // Expected major version for Brave and Chromium.
  const std::string major_version = version_info::GetMajorVersionNumber();

  CheckUserAgentMetadataVersionsList(
      *brands_list, major_version, [](const std::string& version) {
        EXPECT_EQ(std::string::npos, version.find("."));
      });

  // Check full versions
  const base::Value::List* full_version_list =
      values->FindList("fullVersionList");
  ASSERT_NE(nullptr, full_version_list);

  // Expected version string for Brave and Chromium.
  const std::string expected_full_version =
      base::StrCat({major_version, ".0.0.0"});

  CheckUserAgentMetadataVersionsList(
      *full_version_list, expected_full_version,
      [](const std::string& version_str) {
        base::Version version(version_str);
        for (size_t i = 0; i < version.components().size(); i++) {
          if (i > 0) {
            EXPECT_EQ(0U, version.components()[i]);
          }
        }
      });

  // Check auFullVersion
  const std::string* ua_full_version = values->FindString("uaFullVersion");
  ASSERT_NE(nullptr, ua_full_version);
  EXPECT_EQ(expected_full_version, *ua_full_version);
}
