// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/localhost_permission/localhost_permission_component.h"
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
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "net/test/test_data_directory.h"
#include "url/gurl.h"

using net::test_server::EmbeddedTestServer;

namespace {

constexpr char kTestEmbeddingDomain[] = "a.com";
constexpr char kTestTargetPath[] = "/logo.png";
constexpr char kSimplePage[] = "/simple.html";

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
    localhost_permission_component_ =
        g_brave_browser_process->localhost_permission_component();
    if (localhost_permission_component_) {
      localhost_permission_component_->SetAllowedDomainsForTesting(
          {kTestEmbeddingDomain});
    }

    // Embedding website server
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_->ServeFilesFromDirectory(test_data_dir);
    content::SetupCrossSiteRedirector(https_server_.get());
    ASSERT_TRUE(https_server_->Start());
    // Localhost server
    localhost_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    localhost_server_->ServeFilesFromDirectory(test_data_dir);
    content::SetupCrossSiteRedirector(localhost_server_.get());
    ASSERT_TRUE(localhost_server_->Start());

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

  void WaitForAdBlockServiceThreads() {
    auto tr_helper = base::MakeRefCounted<base::ThreadTestHelper>(
        g_brave_browser_process->local_data_files_service()->GetTaskRunner());
    ASSERT_TRUE(tr_helper->Run());
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

  void AddAdblockRule(const std::string& rule) {
    source_provider_ =
        std::make_unique<brave_shields::TestFiltersProvider>(rule);

    brave_shields::AdBlockService* ad_block_service =
        g_brave_browser_process->ad_block_service();
    ad_block_service->UseSourceProviderForTest(source_provider_.get());
    WaitForAdBlockServiceThreads();
  }

  void InsertImage(const std::string& src, const bool expected) {
    std::string insert_image = content::JsReplace(
        R"(
        (async () => {
          console.log("Entered insert image script");
          const img = document.createElement('img');
          img.src = $1;
          document.body.appendChild(img);
          return await new Promise((resolve) => {
            img.addEventListener("load", () => {
              resolve(true);
            }, {once: true});
            img.addEventListener("error", () => {
              resolve(false);
            }, {once: true});
          });
        })();
        )",
        src);
    ASSERT_EQ(expected, EvalJs(contents(), insert_image));
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

  void CheckAskAndAcceptFlow(GURL localhost_url, int prompt_count = 0) {
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    EXPECT_EQ(prompt_count, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Accept prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::ACCEPT_ALL);
    // Load subresource.
    InsertImage(localhost_url.spec(), false);
    // Make sure prompt came up.
    EXPECT_EQ(prompt_count + 1, prompt_factory()->show_count());
    // Check content setting is now ALLOWed.
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ALLOW);
    // Access to localhost resources should be allowed.
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    InsertImage(localhost_url.spec(), true);
    // Not another prompt.
    EXPECT_EQ(prompt_count + 1, prompt_factory()->show_count());
  }

  void CheckAskAndDenyFlow(GURL localhost_url, int prompt_count = 0) {
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    EXPECT_EQ(prompt_count, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Deny prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::DENY_ALL);
    // Load subresource
    InsertImage(localhost_url.spec(), false);
    // Make sure prompt came up.
    EXPECT_EQ(prompt_count + 1, prompt_factory()->show_count());
    // Check content setting is now DENY.
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_BLOCK);
    // Access to localhost resources should be denied.
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    InsertImage(localhost_url.spec(), false);
    // Not another prompt.
    EXPECT_EQ(prompt_count + 1, prompt_factory()->show_count());
  }

  void CheckAskAndDismissFlow(GURL localhost_url, int prompt_count = 0) {
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    EXPECT_EQ(prompt_count, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Dismiss prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::DISMISS);
    // Load subresource
    InsertImage(localhost_url.spec(), false);
    // Make sure prompt came up.
    EXPECT_EQ(prompt_count + 1, prompt_factory()->show_count());
    // Check content setting is still ASK.
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    // Access to localhost resources should be denied.
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    InsertImage(localhost_url.spec(), false);
    // Still ask for prompt.
    EXPECT_EQ(prompt_count + 2, prompt_factory()->show_count());
  }

  void CheckNoPromptFlow(const bool expected, GURL localhost_url) {
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
    EXPECT_EQ(0, prompt_factory()->show_count());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
    // Load subresource
    InsertImage(localhost_url.spec(), expected);
    // No prompt.
    prompt_factory()->set_response_type(
        permissions::PermissionRequestManager::ACCEPT_ALL);
    // Make sure prompt did not come up.
    EXPECT_EQ(0, prompt_factory()->show_count());
    // Check content setting is still ASK.
    CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
  }

 protected:
  GURL embedding_url_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<net::EmbeddedTestServer> localhost_server_;
  base::test::ScopedFeatureList feature_list_;
  raw_ptr<Browser> current_browser_;
  std::unique_ptr<brave_shields::TestFiltersProvider> source_provider_;
  raw_ptr<localhost_permission::LocalhostPermissionComponent>
      localhost_permission_component_;

 private:
  std::unique_ptr<permissions::MockPermissionPromptFactory> prompt_factory_;
};

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, Localhost) {
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(target_url);
  // Reset content setting.
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDenyFlow(target_url, 1);
  // Reset content setting.
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDismissFlow(target_url, 2);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, DotLocalhost) {
  std::string test_domain = "test.localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(target_url);
  // Reset content setting.
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDenyFlow(target_url, 1);
  // Reset content setting.
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDismissFlow(target_url, 2);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, OneTwoSeven) {
  std::string test_domain = "127.0.0.1";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(target_url);
  // Reset content setting
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDenyFlow(target_url, 1);
  // Reset content setting.
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ASK);
  CheckAskAndDismissFlow(target_url, 2);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, IncognitoModeInheritAllow) {
  // Allowed permission for a website is ASK in incognito.
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(target_url);
  // Check incognito mode.
  Profile* profile = browser()->profile();
  Browser* incognito_browser = CreateIncognitoBrowser(profile);
  SetBrowser(incognito_browser);
  CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, IncognitoModeInheritBlock) {
  // Blocked permission for a website is ASK in incognito.
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndDenyFlow(target_url);
  // Check Incognito mode.
  Profile* profile = browser()->profile();
  Browser* incognito_browser = CreateIncognitoBrowser(profile);
  SetBrowser(incognito_browser);
  CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, IncognitoModeDoesNotLeak) {
  // Permission set in Incognito does not leak back to normal mode.
  Browser* original_browser = browser();
  Browser* incognito_browser = CreateIncognitoBrowser();
  SetBrowser(incognito_browser);
  SetPromptFactory(GetPermissionRequestManager());
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(target_url);
  // Check permission did not leak.
  SetBrowser(original_browser);
  SetPromptFactory(GetPermissionRequestManager());
  CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
}

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, NoPermissionPrompt) {
  // No permission prompt is shown when we request non-localhost domain.
  std::string test_domain = "b.com";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckNoPromptFlow(true, target_url);
}

// Test that WebSocket connections to localhost are blocked/allowed.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, WebSocket) {
  // Start a WebSocket server.
  auto ws_server = std::make_unique<net::SpawnedTestServer>(
      net::SpawnedTestServer::TYPE_WSS, net::GetWebSocketTestDataDirectory());
  ASSERT_TRUE(ws_server->Start());
  auto ws_url = ws_server->GetURL("localhost", "echo-with-no-extension");
  // Script to connect to ws server.
  std::string ws_open_script_template = R"(
    new Promise(resolve => {
      let socket = new WebSocket($1);
      socket.addEventListener('open', () => resolve('open'));
      socket.addEventListener('error', () => resolve('error'));
    });
  )";
  // Go to any simple page.
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  prompt_factory()->set_response_type(
      permissions::PermissionRequestManager::ACCEPT_ALL);
  // Run script to open WebSocket, it should error out.
  const std::string& ws_open_script =
      content::JsReplace(ws_open_script_template, ws_url);
  ASSERT_EQ("error", EvalJs(contents(), ws_open_script));
  EXPECT_EQ(1, prompt_factory()->show_count());
  // Wait for tab to reload after permission grant.
  WaitForLoadStop(contents());
  CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ALLOW);
  ASSERT_EQ("open", EvalJs(contents(), ws_open_script));
}

// Test that service worker connections are blocked/allowed correctly.
// Service workers making requests to localhost subresources should be allowed
// if the page has the ALLOW content setting, and blocked otherwise.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, ServiceWorker) {
  std::string test_domain = "localhost";
  embedding_url_ =
      https_server_->GetURL(kTestEmbeddingDomain, "/navigator/simple.html");
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);
  EXPECT_EQ(0, prompt_factory()->show_count());
  // Go to page.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), embedding_url_));
  CheckCurrentStatusIs(ContentSetting::CONTENT_SETTING_ASK);
  // Register service worker that will capture all fetches.
  std::string sw_register_script = R"(
    registerServiceWorker('./service-workers-localhost-permission.js')
  )";
  ASSERT_EQ(true, EvalJs(contents(), sw_register_script));
  // Load subresource - it should fail without prompt
  // because the request goes through the SW.
  InsertImage(target_url.spec(), false);
  EXPECT_EQ(0, prompt_factory()->show_count());
  // Now set the content setting to ALLOW.
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ALLOW);
  // Load subresource, should succeed.
  InsertImage(target_url.spec(), true);
  // Still no prompt though.
  EXPECT_EQ(0, prompt_factory()->show_count());
}

// Test that localhost connections blocked by adblock are still blocked without
// permission prompt.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, AdblockRule) {
  // Add adblock rule to block localhost.
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);
  auto rule = base::StrCat({"||", test_domain, "^"});
  AddAdblockRule(rule);
  // The image won't show up because of adblock rule.
  CheckNoPromptFlow(false, target_url);
}

// Test that badfiltering a localhost adblock rule makes permission come up.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, AdblockRuleBadfilter) {
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);

  auto adblock_rule = base::StrCat({"||", test_domain, "^"});
  auto badfilter_rule = base::StrCat({"\n", adblock_rule, "$badfilter"});
  auto rules = adblock_rule + badfilter_rule;
  AddAdblockRule(rules);
  CheckAskAndAcceptFlow(target_url);
}

// Test that localhost connections from website not on allowlist
// are blocked without permission prompt.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, WebsiteNotOnAllowlist) {
  std::string test_domain = "localhost";
  // Note: we're also testing that comments are handled correctly here
  // because we inserted #b.com into the allowlist.
  localhost_permission_component_->SetAllowedDomainsForTesting(
      {base::StrCat({kTestEmbeddingDomain, "\n", "#b.com"})});
  embedding_url_ = https_server_->GetURL("b.com", kSimplePage);
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);
  CheckNoPromptFlow(false, target_url);
}

// Test that manually adding a website to the site permission exceptions
// allows connections to localhost from that eTLD+1.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest,
                       WebsiteNotOnAllowlistButManuallyAdded) {
  std::string test_domain = "localhost";
  // Clear out the allowlist.
  localhost_permission_component_->SetAllowedDomainsForTesting({""});
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);
  SetCurrentStatus(ContentSetting::CONTENT_SETTING_ALLOW);
  // Load subresource, should succeed.
  InsertImage(target_url.spec(), true);
  // No prompt though.
  EXPECT_EQ(0, prompt_factory()->show_count());
}

// Test that different hosts under the same eTLD+1 can prompt.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, WebsitePartOfETLDP1) {
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(
      base::StrCat({"test1.", kTestEmbeddingDomain}), kSimplePage);
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);
  CheckAskAndAcceptFlow(target_url);
  embedding_url_ = https_server_->GetURL(
      base::StrCat({"test2.", kTestEmbeddingDomain}), kSimplePage);
  CheckAskAndAcceptFlow(target_url, 1);
}

// Test that localhost connections blocked by adblock are still blocked without
// permission prompt, and exceptioned domains cause permission prompt.
IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTest, AdblockRuleException) {
  // Add adblock rule to block localhost.
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url =
      localhost_server_->GetURL(test_domain, kTestTargetPath);
  auto original_rule = base::StrCat({"||", test_domain, "^", "\n"});
  auto exception_rule = base::StrCat({"@@||", test_domain, "^"});
  auto rules = original_rule + exception_rule;
  AddAdblockRule(rules);
  CheckAskAndAcceptFlow(target_url);
}

class LocalhostAccessBrowserTestFeatureDisabled
    : public LocalhostAccessBrowserTest {
 public:
  LocalhostAccessBrowserTestFeatureDisabled() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(
        brave_shields::features::kBraveLocalhostAccessPermission);
  }
};

IN_PROC_BROWSER_TEST_F(LocalhostAccessBrowserTestFeatureDisabled,
                       NoPermissionPrompt) {
  std::string test_domain = "localhost";
  embedding_url_ = https_server_->GetURL(kTestEmbeddingDomain, kSimplePage);
  const auto& target_url = https_server_->GetURL(test_domain, kTestTargetPath);
  CheckNoPromptFlow(true, target_url);
}
