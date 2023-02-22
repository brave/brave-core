// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/path_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/test/mock_permission_prompt_factory.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "url/gurl.h"

using net::test_server::EmbeddedTestServer;

namespace {

const char kTestEmbeddingDomain[] = "a.com";
const char kTestEmbeddingPath[] = "/test.html";
const char kTestTargetPath[] = "/target.html";
const char kTestBodyScaffolding[] =
    R"(
    <html>
    <a href="https://%s:%s/target.html" id="link">link</a>
    </html>
    )";

// Used to identify the buttons on the test page.
const char kButtonHtmlId[] = "link";

}  // namespace

class LocalhostAccessBrowserTest : public InProcessBrowserTest {
 public:
  LocalhostAccessBrowserTest() {
    feature_list_.InitAndEnableFeature(
        brave_shields::features::kBraveLocalhostAccessPermission);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    current_browser_ = InProcessBrowserTest::browser();
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    content::SetupCrossSiteRedirector(https_server_.get());
    permissions::PermissionRequestManager* manager =
        GetPermissionRequestManager();
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(manager);
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  permissions::PermissionRequestManager* GetPermissionRequestManager() {
    return permissions::PermissionRequestManager::FromWebContents(
        browser()->tab_strip_model()->GetActiveWebContents());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
    current_browser_ = InProcessBrowserTest::browser();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void TearDownOnMainThread() override { prompt_factory_.reset(); }

  permissions::MockPermissionPromptFactory* prompt_factory() {
    return prompt_factory_.get();
  }

  Browser* browser() { return current_browser_; }

  void SetBrowser(Browser* browser) { current_browser_ = browser; }

  void SetPromptFactory(permissions::PermissionRequestManager* manager) {
    prompt_factory_ =
        std::make_unique<permissions::MockPermissionPromptFactory>(manager);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ClickButtonWithId(const std::string& id) {
    std::string click_script = content::JsReplace(
        R"(
        new Promise(async (resolve, reject) => {
            try {
              const button = document.getElementById($1);
              button.click();
              resolve(true);
            } catch(error) {
              reject(error);
            }
          })
      )",
        id);
    ASSERT_EQ(true, EvalJs(contents(), click_script));
  }

  void CheckCurrentStatusIs(ContentSetting content_setting) {
    EXPECT_EQ(content_settings()->GetContentSetting(
                  embedding_url_, embedding_url_,
                  ContentSettingsType::BRAVE_LOCALHOST_ACCESS),
              content_setting);
  }

  void SetCurrentStatus(ContentSetting content_setting) {
    content_settings()->SetContentSettingDefaultScope(
        embedding_url_, embedding_url_,
        ContentSettingsType::BRAVE_LOCALHOST_ACCESS, content_setting);
  }

  void CheckAskAndAcceptFlow(std::string button_id,
                             GURL localhost_url,
                             int prompt_count = 0) {
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    EXPECT_EQ(prompt_count, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Accept prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::ACCEPT_ALL);
    ClickButtonWithId(button_id);
    prompt_factory()->WaitForPermissionBubble();
    // Make sure prompt came up.
    EXPECT_EQ(prompt_count + 1, prompt_factory()->show_count());
    // Check content setting is now ALLOWed.
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ALLOW);
    // Access to localhost resources should be allowed.
    content::TestNavigationObserver load_observer(contents());
    ClickButtonWithId(button_id);
    load_observer.Wait();
    EXPECT_TRUE(load_observer.last_navigation_succeeded());
    EXPECT_EQ(localhost_url, load_observer.last_navigation_url());
  }

  void CheckAskAndDenyFlow(std::string button_id,
                           GURL localhost_url,
                           int prompt_count = 0) {
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    EXPECT_EQ(prompt_count, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Deny prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::DENY_ALL);
    ClickButtonWithId(button_id);
    prompt_factory()->WaitForPermissionBubble();
    // Make sure prompt came up.
    EXPECT_EQ(prompt_count + 1, prompt_factory()->show_count());
    // Check content setting is now DENY.
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_BLOCK);
    // Access to localhost resources should be denied.
    content::TestNavigationObserver load_observer(contents());
    ClickButtonWithId(button_id);
    load_observer.Wait();
    EXPECT_FALSE(load_observer.last_navigation_succeeded());
    EXPECT_EQ(localhost_url, load_observer.last_navigation_url());
  }

  void CheckNoPromptFlow(std::string button_id, GURL localhost_url) {
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    EXPECT_EQ(0, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // No prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::DENY_ALL);
    ClickButtonWithId(button_id);
    // Make sure prompt did not come up.
    EXPECT_EQ(0, prompt_factory()->show_count());
    // Check content setting is still ASK.
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
  }

 protected:
  GURL embedding_url_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
  Browser* current_browser_;

 private:
  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;
};

std::unique_ptr<net::test_server::HttpResponse> HandleHttpRequest(
    const std::string& domain,
    const net::test_server::HttpRequest& request) {
  const auto port = request.GetURL().port();
  auto http_response = std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  if (request.GetURL().path() == kTestEmbeddingPath) {
    http_response->set_content(
        base::StringPrintf(kTestBodyScaffolding, domain.c_str(), port.c_str()));
  } else if (request.relative_url == "target.html") {
    // Simple response for localhost.
    http_response->set_content("This is a localhost page");
  }
  return http_response;
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, Localhost) {
  std::string test_domain = "localhost";
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleHttpRequest, test_domain));
  ASSERT_TRUE(https_server_->Start());
  embedding_url_ =
      https_server_->GetURL(kTestEmbeddingDomain, kTestEmbeddingPath);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(kButtonHtmlId, target_url);
  // Reset content setting
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDenyFlow(kButtonHtmlId, target_url, 1);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, DotLocalhost) {
  std::string test_domain = "test.localhost";
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleHttpRequest, test_domain));
  ASSERT_TRUE(https_server_->Start());
  embedding_url_ =
      https_server_->GetURL(kTestEmbeddingDomain, kTestEmbeddingPath);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(kButtonHtmlId, target_url);
  // Reset content setting
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDenyFlow(kButtonHtmlId, target_url, 1);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, OneTwoSeven) {
  std::string test_domain = "127.0.0.1";
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleHttpRequest, test_domain));
  ASSERT_TRUE(https_server_->Start());
  embedding_url_ =
      https_server_->GetURL(kTestEmbeddingDomain, kTestEmbeddingPath);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(kButtonHtmlId, target_url);
  // Reset content setting
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDenyFlow(kButtonHtmlId, target_url, 1);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, NoPermissionPrompt) {
  https_server_->RegisterRequestHandler(
      base::BindRepeating(HandleHttpRequest, "b.com"));
  ASSERT_TRUE(https_server_->Start());
  embedding_url_ =
      https_server_->GetURL(kTestEmbeddingDomain, kTestEmbeddingPath);
  const auto& target_url = https_server_->GetURL("b.com", kTestTargetPath);
  CheckNoPromptFlow(kButtonHtmlId, target_url);
}
