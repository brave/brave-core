/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

namespace {

const char kEmbeddedTestServerDirectory[] = "brave-wallet";

std::string CheckForEventScript(const std::string& event_var) {
  return base::StringPrintf(R"(function waitForEvent() {
             if (%s) {
               window.domAutomationController.send(true);
             } else {
               if (window.ethereum) {
                 if (!set_events_listeners) {
                   set_events_listeners = true
                   window.ethereum.on('connect', function(chainId) {
                     received_connect_event = true
                   });
                   window.ethereum.on('chainChanged', function(chainId) {
                     received_chain_changed_event = true
                   });
                 }
                 if (window.ethereum.isConnected()) {
                   received_connect_event = true
                   received_chain_changed_event = true
                 }
               }
               setTimeout(waitForEvent, 100);
             }
            }
            var set_events_listeners = false;
            waitForEvent();)",
                            event_var.c_str());
}

}  // namespace

class BraveWalletEventEmitterTest : public InProcessBrowserTest {
 public:
  BraveWalletEventEmitterTest() {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII(kEmbeddedTestServerDirectory);
    https_server_->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  brave_wallet::BraveWalletService* GetBraveWalletService() {
    brave_wallet::BraveWalletService* service =
        brave_wallet::BraveWalletServiceFactory::GetInstance()->GetForContext(
            browser()->profile());
    EXPECT_TRUE(service);
    return service;
  }

  brave_wallet::EthJsonRpcController* GetEthJsonRpcController() {
    return GetBraveWalletService()->rpc_controller();
  }

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletEventEmitterTest, CheckForAConnectEvent) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_event_emitter.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto result_first = EvalJsWithManualReply(
      contents, CheckForEventScript("received_connect_event"));
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEventEmitterTest,
                       CheckForAChainChangedEvent) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_event_emitter.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  auto* controller = GetEthJsonRpcController();
  controller->SetNetwork(brave_wallet::Network::kGoerli);

  auto result_first = EvalJsWithManualReply(
      contents, CheckForEventScript("received_chain_changed_event"));
  EXPECT_EQ(base::Value(true), result_first.value);
}
