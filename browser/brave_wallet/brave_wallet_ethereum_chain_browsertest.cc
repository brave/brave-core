/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

namespace {

const char kEmbeddedTestServerDirectory[] = "brave-wallet";

const char kScriptWaitForEvent[] = R"(
    function waitForEvent() {
        if (request_finished) {
          console.log('!!!send true')
          window.domAutomationController.send(chain_added_result);
        }
    }
    console.log('!!!starting')
    setInterval(waitForEvent, 100);)";

const char kScriptRunAndCheckAddChainResult[] = R"(
      function waitForEvent() {
        if (!window.ethereum)
          return;
        window.ethereum.request({ method: 'wallet_addEthereumChain', params:[{
              chainId: '%s',
              chainName: 'Test Smart Chain',
              rpcUrls: ['https://bsc-dataseed.binance.org/'],
          }]
        }).then(result => {
          console.log(JSON.stringify(result));
          %s
        }).catch(result => {
          console.log(JSON.stringify(result));
          %s
        })
      };
      console.log("!!!starting");
      setInterval(waitForEvent, 100);
    )";

const char kRejectedResult[] =
    R"(window.domAutomationController.send(
        result.error && result.error.code == 4001))";

}  // namespace

class BraveWalletEthereumChainTest : public InProcessBrowserTest {
 public:
  BraveWalletEthereumChainTest() {
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

  mojo::Remote<brave_wallet::mojom::EthJsonRpcController>
  GetEthJsonRpcController() {
    if (!rpc_controller_) {
      auto pending =
          brave_wallet::RpcControllerFactory::GetInstance()->GetForContext(
              browser()->profile());
      rpc_controller_.Bind(std::move(pending));
    }
    return std::move(rpc_controller_);
  }

 private:
  mojo::Remote<brave_wallet::mojom::EthJsonRpcController> rpc_controller_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddEthereumChainApproved) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_ethereum_chain.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  ASSERT_TRUE(brave_wallet::BraveWalletTabHelper::FromWebContents(contents)
                  ->IsShowingBubble());
  GetEthJsonRpcController()->PendingRequestCompleted("0x38", true);
  auto result_first = EvalJs(contents, kScriptWaitForEvent,
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddEthereumChainRejected) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_ethereum_chain.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  ASSERT_TRUE(brave_wallet::BraveWalletTabHelper::FromWebContents(contents)
                  ->IsShowingBubble());
  GetEthJsonRpcController()->PendingRequestCompleted("0x38", false);
  auto result_first = EvalJs(contents, kScriptWaitForEvent,
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddChainSameOrigin) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_ethereum_chain.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contents);
  ASSERT_TRUE(tab_helper->IsShowingBubble());
  tab_helper->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  auto script = base::StringPrintf(kScriptRunAndCheckAddChainResult, "0x11", "",
                                   kRejectedResult);
  auto result_first =
      EvalJs(contents, script, content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddChainDifferentOrigins) {
  GURL urlA =
      https_server()->GetURL("a.com", "/brave_wallet_ethereum_chain.html");
  ui_test_utils::NavigateToURL(browser(), urlA);
  content::WebContents* contentsA =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contentsA);
  auto* tab_helperA =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contentsA);
  ASSERT_TRUE(tab_helperA->IsShowingBubble());
  tab_helperA->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helperA->IsShowingBubble());

  chrome::NewTab(browser());
  auto* web_contentsB = browser()->tab_strip_model()->GetWebContentsAt(1);
  GURL urlB =
      https_server()->GetURL("b.com", "/brave_wallet_ethereum_chain.html");
  // Put same chain in new origin
  EXPECT_TRUE(content::NavigateToURL(web_contentsB, urlB));
  WaitForLoadStop(web_contentsB);
  auto* tab_helperB =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contentsB);
  ASSERT_FALSE(tab_helperB->IsShowingBubble());
  auto rejected_same_id = EvalJs(web_contentsB, kScriptWaitForEvent,
                                 content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), rejected_same_id.value);
  ASSERT_FALSE(tab_helperB->IsShowingBubble());
  ASSERT_FALSE(tab_helperA->IsShowingBubble());
}
