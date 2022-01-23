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
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
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
#include "url/origin.h"
#include "url/url_util.h"

namespace {

const char kEmbeddedTestServerDirectory[] = "brave-wallet";

const char kScriptWaitForEvent[] = R"(
    function waitForEvent() {
        if (request_finished) {
          window.domAutomationController.send(chain_added_result);
        }
    }
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
          %s
        }).catch(result => {
          %s
        })
      };
      console.log("!!!starting");
      setInterval(waitForEvent, 100);
    )";

const char kScriptRunEmptyAndCheckChainResult[] = R"(
      function waitForEvent() {
        if (!window.ethereum)
          return;
        window.ethereum.request({ method: 'wallet_addEthereumChain', params:[]
        }).catch(result => {
          %s
        })
      };
      console.log("!!!starting");
      setInterval(waitForEvent, 100);
    )";

const char kRejectedResult[] =
    R"(window.domAutomationController.send(
        result.code == %s))";

std::string EncodeQuery(const std::string& query) {
  url::RawCanonOutputT<char> buffer;
  url::EncodeURIComponent(query.data(), query.size(), &buffer);
  return std::string(buffer.data(), buffer.length());
}

void ExtractParameters(const std::string& params,
                       std::map<std::string, std::string>* result) {
  for (const std::string& pair : base::SplitString(
           params, "&", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    std::vector<std::string> key_val = base::SplitString(
        pair, "=", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (!key_val.empty()) {
      std::string key = key_val[0];
      EXPECT_TRUE(result->find(key) == result->end());
      (*result)[key] = (key_val.size() == 2) ? key_val[1] : std::string();
    } else {
      NOTREACHED();
    }
  }
}

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
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&BraveWalletEthereumChainTest::HandleChainRequest,
                            base::Unretained(this)));

    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleChainRequest(
      const net::test_server::HttpRequest& request) {
    GURL absolute_url = https_server_->GetURL(request.relative_url);
    if (absolute_url.path() != "/rpc")
      return nullptr;
    std::map<std::string, std::string> params;
    ExtractParameters(absolute_url.query(), &params);
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    auto chain_id = params.size() ? params["id"] : "0x38";
    http_response->set_content("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"" +
                               chain_id + "\"}");
    return http_response;
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  GURL GetWalletEthereumChainPageURL(const std::string& host = "a.com",
                                     const std::string& query = "") {
    GURL rpc = https_server()->GetURL("c.com", "/rpc");
    if (!query.empty()) {
      GURL::Replacements replacements;
      replacements.SetQueryStr(query);
      rpc = rpc.ReplaceComponents(replacements);
    }
    std::string rpc_query("rpc=" + EncodeQuery(rpc.spec()));
    GURL::Replacements replacements;
    if (!query.empty())
      rpc_query += "&" + query;
    replacements.SetQueryStr(rpc_query);
    auto url =
        https_server()->GetURL(host, "/brave_wallet_ethereum_chain.html");
    return url.ReplaceComponents(replacements);
  }

  brave_wallet::JsonRpcService* GetJsonRpcService() {
    return brave_wallet::JsonRpcServiceFactory::GetInstance()
        ->GetServiceForContext(browser()->profile());
  }

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddEthereumChainApproved) {
  std::vector<brave_wallet::mojom::EthereumChainPtr> result;
  auto* prefs = browser()->profile()->GetPrefs();
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_TRUE(result.empty());
  GURL url = GetWalletEthereumChainPageURL();
  base::RunLoop loop;
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contents);
  tab_helper->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(contents);
  loop.Run();

  ASSERT_TRUE(brave_wallet::BraveWalletTabHelper::FromWebContents(contents)
                  ->IsShowingBubble());
  GetJsonRpcService()->AddEthereumChainRequestCompleted("0x38", true);
  base::RunLoop().RunUntilIdle();  // For FirePendingRequestCompleted
  url::Origin url_origin = url::Origin::Create(url);
  GetJsonRpcService()->NotifySwitchChainRequestProcessed(true,
                                                         url_origin.GetURL());
  auto result_first = EvalJs(contents, kScriptWaitForEvent,
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result_first.value);
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_FALSE(result.empty());
  EXPECT_EQ(result.front()->chain_id, "0x38");
  EXPECT_EQ(GetJsonRpcService()->GetChainId(), "0x38");
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddEthereumChainRejected) {
  GURL url = GetWalletEthereumChainPageURL();
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  base::RunLoop loop;
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contents);
  tab_helper->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(contents);
  loop.Run();
  ASSERT_TRUE(brave_wallet::BraveWalletTabHelper::FromWebContents(contents)
                  ->IsShowingBubble());
  GetJsonRpcService()->AddEthereumChainRequestCompleted("0x38", false);
  auto result_first = EvalJs(contents, kScriptWaitForEvent,
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddChainSameOrigin) {
  GURL url = GetWalletEthereumChainPageURL();
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  base::RunLoop loop;
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contents);
  tab_helper->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(contents);
  loop.Run();
  ASSERT_TRUE(tab_helper->IsShowingBubble());
  tab_helper->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  auto script =
      base::StringPrintf(kScriptRunAndCheckAddChainResult, "0x11", "",
                         base::StringPrintf(kRejectedResult, "4001").c_str());
  auto result_first =
      EvalJs(contents, script, content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest,
                       AddSameChainDifferentOrigins) {
  GURL urlA = GetWalletEthereumChainPageURL();
  content::WebContents* contentsA =
      browser()->tab_strip_model()->GetActiveWebContents();
  base::RunLoop loop;
  auto* tab_helperA =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contentsA);
  tab_helperA->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), urlA));
  WaitForLoadStop(contentsA);
  loop.Run();
  ASSERT_TRUE(tab_helperA->IsShowingBubble());
  tab_helperA->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helperA->IsShowingBubble());

  chrome::NewTab(browser());
  auto* web_contentsB = browser()->tab_strip_model()->GetWebContentsAt(1);
  GURL urlB = GetWalletEthereumChainPageURL("b.com");
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

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest,
                       AddDifferentChainsNoSwitch) {
  std::vector<brave_wallet::mojom::EthereumChainPtr> result;
  auto* prefs = browser()->profile()->GetPrefs();
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_TRUE(result.empty());

  GURL urlA = GetWalletEthereumChainPageURL();
  content::WebContents* contentsA =
      browser()->tab_strip_model()->GetActiveWebContents();
  base::RunLoop loop;
  auto* tab_helperA =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contentsA);
  tab_helperA->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), urlA));
  WaitForLoadStop(contentsA);
  loop.Run();
  ASSERT_TRUE(tab_helperA->IsShowingBubble());
  tab_helperA->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helperA->IsShowingBubble());

  chrome::NewTab(browser());
  auto* web_contentsB = browser()->tab_strip_model()->GetWebContentsAt(1);
  GURL urlB = GetWalletEthereumChainPageURL("b.com", "id=0x11");

  base::RunLoop loopB;
  auto* tab_helperB =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contentsB);
  tab_helperB->SetShowBubbleCallbackForTesting(loopB.QuitClosure());
  // Put same chain in new origin
  EXPECT_TRUE(content::NavigateToURL(web_contentsB, urlB));
  WaitForLoadStop(web_contentsB);
  loopB.Run();

  ASSERT_TRUE(tab_helperB->IsShowingBubble());

  // Add Ethereum chain but don't switch
  GetJsonRpcService()->AddEthereumChainRequestCompleted("0x11", true);
  base::RunLoop().RunUntilIdle();  // For FirePendingRequestCompleted
  url::Origin urlB_origin = url::Origin::Create(urlB);
  GetJsonRpcService()->NotifySwitchChainRequestProcessed(false,
                                                         urlB_origin.GetURL());
  auto rejected_same_id = EvalJs(web_contentsB, kScriptWaitForEvent,
                                 content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), rejected_same_id.value);
  base::RunLoop().RunUntilIdle();
  // Chain should still exist though
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_FALSE(result.empty());
  EXPECT_EQ(result.front()->chain_id, "0x11");
  // But current chain should not change
  EXPECT_EQ(GetJsonRpcService()->GetChainId(), "0x1");
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddDifferentChainsSwitch) {
  std::vector<brave_wallet::mojom::EthereumChainPtr> result;
  auto* prefs = browser()->profile()->GetPrefs();
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_TRUE(result.empty());

  GURL urlA = GetWalletEthereumChainPageURL();

  base::RunLoop loopA;
  content::WebContents* contentsA =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto* tab_helperA =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contentsA);
  tab_helperA->SetShowBubbleCallbackForTesting(loopA.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), urlA));
  WaitForLoadStop(contentsA);
  loopA.Run();
  ASSERT_TRUE(tab_helperA->IsShowingBubble());
  tab_helperA->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helperA->IsShowingBubble());

  chrome::NewTab(browser());
  auto* web_contentsB = browser()->tab_strip_model()->GetWebContentsAt(1);
  GURL urlB = GetWalletEthereumChainPageURL("b.com", "id=0x11");

  base::RunLoop loop;
  auto* tab_helperB =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contentsB);
  tab_helperB->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  // Put same chain in new origin
  EXPECT_TRUE(content::NavigateToURL(web_contentsB, urlB));
  WaitForLoadStop(web_contentsB);
  loop.Run();
  ASSERT_TRUE(tab_helperB->IsShowingBubble());

  // Add Ethereum chain and switch
  GetJsonRpcService()->AddEthereumChainRequestCompleted("0x11", true);
  base::RunLoop().RunUntilIdle();  // For FirePendingRequestCompleted
  url::Origin urlB_origin = url::Origin::Create(urlB);
  GetJsonRpcService()->NotifySwitchChainRequestProcessed(true,
                                                         urlB_origin.GetURL());
  auto rejected_same_id = EvalJs(web_contentsB, kScriptWaitForEvent,
                                 content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), rejected_same_id.value);
  base::RunLoop().RunUntilIdle();
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_FALSE(result.empty());
  EXPECT_EQ(result.front()->chain_id, "0x11");
  EXPECT_EQ(GetJsonRpcService()->GetChainId(), "0x11");
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddChainAndCloseTab) {
  GURL urlA = GetWalletEthereumChainPageURL();
  content::WebContents* contentsA =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto* tab_helperA =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contentsA);
  base::RunLoop loopA;
  tab_helperA->SetShowBubbleCallbackForTesting(loopA.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), urlA));
  WaitForLoadStop(contentsA);
  loopA.Run();
  ASSERT_TRUE(tab_helperA->IsShowingBubble());
  tab_helperA->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helperA->IsShowingBubble());

  chrome::NewTab(browser());
  auto* web_contentsB = browser()->tab_strip_model()->GetWebContentsAt(1);
  GURL urlB = GetWalletEthereumChainPageURL("b.com", "id=0x11");

  base::RunLoop loop;
  auto* tab_helperB =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contentsB);
  tab_helperB->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  EXPECT_TRUE(content::NavigateToURL(web_contentsB, urlB));
  WaitForLoadStop(web_contentsB);
  loop.Run();

  ASSERT_TRUE(tab_helperB->IsShowingBubble());
  browser()->tab_strip_model()->CloseSelectedTabs();
  std::vector<brave_wallet::mojom::EthereumChainPtr> result;
  auto* prefs = browser()->profile()->GetPrefs();
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_TRUE(result.empty());
  GetJsonRpcService()->AddEthereumChainRequestCompleted("0x11", true);
  base::RunLoop().RunUntilIdle();
  brave_wallet::GetAllCustomChains(prefs, &result);
  ASSERT_FALSE(result.empty());
  EXPECT_EQ(result.front()->chain_id, "0x11");
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddBrokenChain) {
  GURL url = GetWalletEthereumChainPageURL();
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  base::RunLoop loop;
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contents);
  tab_helper->SetShowBubbleCallbackForTesting(loop.QuitClosure());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(contents);
  loop.Run();
  ASSERT_TRUE(tab_helper->IsShowingBubble());
  tab_helper->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  auto script =
      base::StringPrintf(kScriptRunEmptyAndCheckChainResult,
                         base::StringPrintf(kRejectedResult, "-32602").c_str());
  auto result_first =
      EvalJs(contents, script, content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  EXPECT_EQ(base::Value(true), result_first.value);
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, CheckIncognitoTab) {
  GURL url = GetWalletEthereumChainPageURL();
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(private_browser, url));
  content::WebContents* contents =
      private_browser->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);
  EXPECT_EQ(url, contents->GetURL());
  base::RunLoop().RunUntilIdle();
  std::string title;
  ASSERT_TRUE(
      ExecuteScriptAndExtractString(contents,
                                    "window.domAutomationController.send("
                                    "document.title)",
                                    &title));
  EXPECT_EQ(title, "PAGE_SCRIPT_STARTED");
  auto result_first = EvalJs(contents,
                             "window.domAutomationController.send("
                             "window.ethereum != null)",
                             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), result_first.value);
}
