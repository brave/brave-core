/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/embedder_support/user_agent_utils.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"

using brave_shields::ControlType;
using content::TitleWatcher;

const char kUserAgentScript[] =
    "domAutomationController.send(navigator.userAgent);";

class BraveNavigatorUserAgentFarblingBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        &BraveNavigatorUserAgentFarblingBrowserTest::MonitorHTTPRequest,
        base::Unretained(this)));
    user_agents_.clear();

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

  void MonitorHTTPRequest(const net::test_server::HttpRequest& request) {
    user_agents_.push_back(request.headers.at("user-agent"));
  }

  std::string last_requested_http_user_agent() {
    if (user_agents_.empty())
      return "";
    return user_agents_[user_agents_.size() - 1];
  }

  std::string effective_user_agent() {
    return browser_content_client_->GetEffectiveUserAgent(
        browser()->profile(), contents()->GetLastCommittedURL());
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

  template <typename T>
  int ExecScriptGetInt(const std::string& script, T* frame) {
    int value;
    EXPECT_TRUE(ExecuteScriptAndExtractInt(frame, script, &value));
    return value;
  }

  template <typename T>
  std::string ExecScriptGetStr(const std::string& script, T* frame) {
    std::string value;
    EXPECT_TRUE(ExecuteScriptAndExtractString(frame, script, &value));
    return value;
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(contents());
  }

 private:
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
  std::vector<std::string> user_agents_;
};

// Tests results of farbling user agent
IN_PROC_BROWSER_TEST_F(BraveNavigatorUserAgentFarblingBrowserTest,
                       FarbleNavigatorUserAgent) {
  base::string16 expected_title = base::ASCIIToUTF16("pass");
  std::string domain_b = "b.com";
  std::string domain_z = "z.com";
  GURL url_b = embedded_test_server()->GetURL(domain_b, "/simple.html");
  GURL url_z = embedded_test_server()->GetURL(domain_z, "/simple.html");
  // get real navigator.userAgent
  std::string real_ua = embedder_support::GetUserAgent();
  // Farbling level: off
  AllowFingerprinting(domain_b);
  NavigateToURLUntilLoadStop(url_b);
  EXPECT_EQ(last_requested_http_user_agent(), real_ua);
  std::string off_ua_b = ExecScriptGetStr(kUserAgentScript, contents());
  // user agent should be the same as the real user agent
  EXPECT_EQ(off_ua_b, real_ua);
  AllowFingerprinting(domain_z);
  NavigateToURLUntilLoadStop(url_z);
  // HTTP User-Agent header we just sent in that request should be the same as
  // the real user agent
  EXPECT_EQ(last_requested_http_user_agent(), real_ua);
  std::string off_ua_z = ExecScriptGetStr(kUserAgentScript, contents());
  // user agent should be the same on every domain if farbling is off
  EXPECT_EQ(off_ua_z, real_ua);

  // Farbling level: default
  // navigator.userAgent may be farbled, but the farbling is not
  // domain-specific
  SetFingerprintingDefault(domain_b);
  NavigateToURLUntilLoadStop(url_b);
  std::string default_ua_b = ExecScriptGetStr(kUserAgentScript, contents());
  SetFingerprintingDefault(domain_z);
  NavigateToURLUntilLoadStop(url_z);
  std::string default_ua_z = ExecScriptGetStr(kUserAgentScript, contents());
  // user agent should be the same on every domain if farbling is default
  EXPECT_EQ(default_ua_b, default_ua_z);

  // Farbling level: maximum
  // navigator.userAgent should be the possibly-farbled string from the default
  // farbling level, further suffixed by a pseudo-random number of spaces based
  // on domain and session key
  BlockFingerprinting(domain_b);
  // test known values
  NavigateToURLUntilLoadStop(url_b);
  std::string max_ua_b = ExecScriptGetStr(kUserAgentScript, contents());
  EXPECT_EQ(max_ua_b, default_ua_b + "   ");
  BlockFingerprinting(domain_z);
  NavigateToURLUntilLoadStop(url_z);
  std::string max_ua_z = ExecScriptGetStr(kUserAgentScript, contents());
  EXPECT_EQ(max_ua_z, default_ua_z + "  ");

  // test that iframes also inherit the farbled user agent
  // (farbling level is still maximum)
  NavigateToURLUntilLoadStop(embedded_test_server()->GetURL(
      domain_b, "/navigator/ua-local-iframe.html"));
  TitleWatcher watcher1(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher1.WaitAndGetTitle());
  NavigateToURLUntilLoadStop(embedded_test_server()->GetURL(
      domain_b, "/navigator/ua-remote-iframe.html"));
  TitleWatcher watcher2(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher2.WaitAndGetTitle());

  // test that workers also inherit the farbled user agent
  // (farbling level is still maximum)
  NavigateToURLUntilLoadStop(embedded_test_server()->GetURL(
      domain_b, "/navigator/workers-useragent.html"));
  // HTTP User-Agent header we just sent in that request should be the same as
  // the real user agent
  EXPECT_EQ(last_requested_http_user_agent(), effective_user_agent());
  TitleWatcher watcher3(contents(), expected_title);
  EXPECT_EQ(expected_title, watcher3.WaitAndGetTitle());

  // Farbling level: off
  // verify that user agent is reset properly after having been farbled
  AllowFingerprinting(domain_b);
  NavigateToURLUntilLoadStop(url_b);
  EXPECT_EQ(last_requested_http_user_agent(), real_ua);
  std::string off_ua_b2 = ExecScriptGetStr(kUserAgentScript, contents());
  EXPECT_EQ(off_ua_b, off_ua_b2);
}
