/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>

#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "brave-wallet";

std::string CheckForEventScript(const std::string& event_var) {
  return base::StringPrintf(R"(
      new Promise(resolve => {
        const timer = setInterval(function () {
          if (%s) {
            clearInterval(timer);
            resolve(true);
          }
        }, 100);
      });
    )",
                            event_var.c_str());
}

}  // namespace

namespace brave_wallet {

class BraveWalletEventEmitterTest : public InProcessBrowserTest {
 public:
  BraveWalletEventEmitterTest() {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    brave_wallet::SetDefaultEthereumWallet(
        browser()->profile()->GetPrefs(),
        brave_wallet::mojom::DefaultWallet::BraveWallet);
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_->ServeFilesFromDirectory(test_data_dir);

    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser()->profile());
    keyring_service_ = brave_wallet_service_->keyring_service();

    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  mojo::Remote<brave_wallet::mojom::JsonRpcService> GetJsonRpcService() {
    if (!json_rpc_service_) {
      mojo::PendingRemote<brave_wallet::mojom::JsonRpcService> remote;
      brave_wallet_service_->json_rpc_service()->Bind(
          remote.InitWithNewPipeAndPassReceiver());
      json_rpc_service_.Bind(std::move(remote));
    }
    return std::move(json_rpc_service_);
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

  AccountUtils GetAccountUtils() { return AccountUtils(keyring_service_); }

  void RestoreWallet() {
    ASSERT_TRUE(keyring_service_->RestoreWalletSync(
        kMnemonicDripCaution, kTestWalletPassword, false));
  }

  void SetSelectedAccount(const mojom::AccountIdPtr& account_id) {
    ASSERT_TRUE(keyring_service_->SetSelectedAccountSync(account_id.Clone()));
  }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  mojo::Remote<brave_wallet::mojom::JsonRpcService> json_rpc_service_;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletEventEmitterTest, CheckForAConnectEvent) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_event_emitter.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  auto result_first =
      EvalJs(contents, CheckForEventScript("received_connect_event"));
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEventEmitterTest,
                       CheckForAChainChangedEvent) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_event_emitter.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto service = GetJsonRpcService();
  service->SetNetwork(brave_wallet::mojom::kSepoliaChainId,
                      brave_wallet::mojom::CoinType::ETH, std::nullopt,
                      base::DoNothing());

  auto result_first =
      EvalJs(contents, CheckForEventScript("received_chain_changed_event"));
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEventEmitterTest,
                       CheckForAnAccountChangedEvent) {
  RestoreWallet();
  auto eth_account = GetAccountUtils().EnsureEthAccount(0);
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_event_emitter.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  url::Origin sub_request_origin;
  ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(
      permissions::RequestType::kBraveEthereum, GetLastCommitedOrigin(),
      eth_account->address, &sub_request_origin));
  host_content_settings_map()->SetContentSettingDefaultScope(
      sub_request_origin.GetURL(), GetLastCommitedOrigin().GetURL(),
      ContentSettingsType::BRAVE_ETHEREUM,
      ContentSetting::CONTENT_SETTING_ALLOW);
  SetSelectedAccount(eth_account->account_id);

  auto result_first =
      EvalJs(contents, CheckForEventScript("received_account_changed_event"));
  EXPECT_EQ(base::Value(true), result_first.value);
}

}  // namespace brave_wallet
