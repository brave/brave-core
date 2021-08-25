/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace permissions {

namespace {

void OnGetAllowedAccountsResult(
    bool* was_called,
    bool expected_success,
    const std::vector<std::string>& expected_allowed_accounts,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  ASSERT_FALSE(*was_called);
  *was_called = true;
  EXPECT_EQ(expected_success, success);
  EXPECT_EQ(expected_allowed_accounts, allowed_accounts);
}

}  // namespace

class BraveEthereumPermissionContextBrowserTest : public InProcessBrowserTest {
 public:
  BraveEthereumPermissionContextBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  ~BraveEthereumPermissionContextBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server()->ServeFilesFromDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(https_server()->Start());
  }

  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  GURL GetLastCommitedOrigin() {
    return web_contents()->GetLastCommittedURL().GetOrigin();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 protected:
  net::test_server::EmbeddedTestServer https_server_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveEthereumPermissionContextBrowserTest,
                       GetAllowedAccounts) {
  std::vector<std::string> addresses = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8C"};

  // Fail if rfh is null.
  bool was_called = false;
  BraveEthereumPermissionContext::GetAllowedAccounts(
      nullptr /* rfh */, addresses,
      base::BindOnce(&OnGetAllowedAccountsResult, &was_called, false,
                     std::vector<std::string>()));
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(was_called);

  was_called = false;
  const GURL& url = https_server()->GetURL("a.com", "/empty.html");
  ui_test_utils::NavigateToURL(browser(), url);

  // No allowed accounts before setting permissions.
  BraveEthereumPermissionContext::GetAllowedAccounts(
      web_contents()->GetMainFrame(), addresses,
      base::BindOnce(&OnGetAllowedAccountsResult, &was_called, true,
                     std::vector<std::string>()));

  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(was_called);

  // Return allowed accounts after permissions are set.
  was_called = false;
  std::vector<std::string> expected_allowed_accounts = {addresses[0],
                                                        addresses[2]};
  for (const auto& account : expected_allowed_accounts) {
    GURL sub_request_origin;
    ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(
        GetLastCommitedOrigin(), account, &sub_request_origin));
    host_content_settings_map()->SetContentSettingDefaultScope(
        sub_request_origin, GetLastCommitedOrigin(),
        ContentSettingsType::BRAVE_ETHEREUM,
        ContentSetting::CONTENT_SETTING_ALLOW);
  }
  BraveEthereumPermissionContext::GetAllowedAccounts(
      web_contents()->GetMainFrame(), addresses,
      base::BindOnce(&OnGetAllowedAccountsResult, &was_called, true,
                     expected_allowed_accounts));
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(was_called);
}

IN_PROC_BROWSER_TEST_F(BraveEthereumPermissionContextBrowserTest,
                       GetAllowedAccountsBlock3PIframe) {
  std::vector<std::string> addresses = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"};

  GURL top_url(https_server()->GetURL("a.com", "/iframe.html"));
  ui_test_utils::NavigateToURL(browser(), top_url);
  GURL iframe_url(https_server()->GetURL("b.com", "/"));
  EXPECT_TRUE(NavigateIframeToURL(web_contents(), "test", iframe_url));

  bool was_called = false;
  auto* iframe_rfh = ChildFrameAt(web_contents()->GetMainFrame(), 0);
  BraveEthereumPermissionContext::GetAllowedAccounts(
      iframe_rfh, addresses,
      base::BindOnce(&OnGetAllowedAccountsResult, &was_called, false,
                     std::vector<std::string>()));
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(was_called);
}

}  // namespace permissions
