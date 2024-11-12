/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace permissions {

class BraveWalletPermissionContextBrowserTest : public InProcessBrowserTest {
 public:
  BraveWalletPermissionContextBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  ~BraveWalletPermissionContextBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server()->ServeFilesFromSourceDirectory(GetChromeTestDataDir());
    ASSERT_TRUE(https_server()->Start());
  }

  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  url::Origin GetLastCommitedOrigin() {
    return url::Origin::Create(web_contents()->GetLastCommittedURL());
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletPermissionContextBrowserTest,
                       GetAllowedAccounts) {
  std::vector<std::string> addresses = {
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B",
      "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8C"};

  // Fail if rfh is null.
  auto allowed_accounts = BraveWalletPermissionContext::GetAllowedAccounts(
      blink::PermissionType::BRAVE_ETHEREUM, nullptr /* rfh */, addresses);
  EXPECT_FALSE(allowed_accounts);

  const GURL& url = https_server()->GetURL("a.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  // No allowed accounts before setting permissions.
  allowed_accounts = BraveWalletPermissionContext::GetAllowedAccounts(
      blink::PermissionType::BRAVE_ETHEREUM,
      web_contents()->GetPrimaryMainFrame(), addresses);
  EXPECT_TRUE(allowed_accounts);
  EXPECT_TRUE(allowed_accounts->empty());

  // Return allowed accounts after permissions are set.
  std::vector<std::string> expected_allowed_accounts = {addresses[0],
                                                        addresses[2]};
  for (const auto& account : expected_allowed_accounts) {
    auto sub_request_origin = brave_wallet::GetSubRequestOrigin(
        permissions::RequestType::kBraveEthereum, GetLastCommitedOrigin(),
        account);
    ASSERT_TRUE(sub_request_origin);
    host_content_settings_map()->SetContentSettingDefaultScope(
        sub_request_origin->GetURL(), GetLastCommitedOrigin().GetURL(),
        ContentSettingsType::BRAVE_ETHEREUM,
        ContentSetting::CONTENT_SETTING_ALLOW);
  }
  allowed_accounts = BraveWalletPermissionContext::GetAllowedAccounts(
      blink::PermissionType::BRAVE_ETHEREUM,
      web_contents()->GetPrimaryMainFrame(), addresses);
  EXPECT_TRUE(allowed_accounts);
  EXPECT_EQ(*allowed_accounts, expected_allowed_accounts);
}

}  // namespace permissions
