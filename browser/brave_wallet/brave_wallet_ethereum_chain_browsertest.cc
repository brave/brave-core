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
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/brave_paths.h"
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
#include "content/public/test/content_mock_cert_verifier.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "url/origin.h"
#include "url/url_util.h"

namespace {

const char kEmbeddedTestServerDirectory[] = "brave-wallet";
const char kSomeChainId[] = "0xabcde";

const char kScriptWaitForEvent[] = R"(
    new Promise(resolve => {
      const timer = setInterval(function () {
        if (request_finished) {
          clearInterval(timer);
          resolve(chain_added_result);
        }
      }, 100);
    });
  )";

const char kScriptRunAndCheckAddChainResult[] = R"(
    new Promise(resolve => {
      const timer = setInterval(function () {
        if (!window.ethereum)
          return;

        window.ethereum.request({ method: 'wallet_addEthereumChain', params:[{
          chainId: '0x11',
          chainName: 'Test Smart Chain',
          rpcUrls: ['https://bsc-dataseed.binance.org/'],
        }]
        }).then(result => {
        }).catch(result => {
          clearInterval(timer);
          resolve(result.code == 4001)
        })
      }, 100);
    });
  )";

const char kScriptRunEmptyAndCheckChainResult[] = R"(
    new Promise(resolve => {
      const timer = setInterval(function () {
        if (!window.ethereum)
          return;
        window.ethereum.request({ method: 'wallet_addEthereumChain', params:[]})
          .catch(result => {
            clearInterval(timer);
            resolve(result.code == -32602)
        })
      }, 100);
    });
  )";

std::string EncodeQuery(const std::string& query) {
  url::RawCanonOutputT<char> buffer;
  url::EncodeURIComponent(query, &buffer);
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
      NOTREACHED_IN_MIGRATION();
    }
  }
}

}  // namespace

class TestJsonRpcServiceObserver
    : public brave_wallet::mojom::JsonRpcServiceObserver {
 public:
  TestJsonRpcServiceObserver(base::OnceClosure callback,
                             const std::string& expected_chain_id,
                             brave_wallet::mojom::CoinType expected_coin,
                             const std::string& expected_error) {
    callback_ = std::move(callback);
    expected_chain_id_ = expected_chain_id;
    expected_coin_ = expected_coin;
    expected_error_ = expected_error;
  }

  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override {
    EXPECT_EQ(chain_id, expected_chain_id_);
    EXPECT_EQ(error, expected_error_);
    std::move(callback_).Run();
  }

  void ChainChangedEvent(const std::string& chain_id,
                         brave_wallet::mojom::CoinType coin,
                         const std::optional<::url::Origin>& origin) override {
    chain_changed_called_ = true;
    EXPECT_EQ(chain_id, expected_chain_id_);
    EXPECT_EQ(coin, expected_coin_);
  }

  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}

  bool chain_changed_called() {
    base::RunLoop().RunUntilIdle();
    return chain_changed_called_;
  }

  ::mojo::PendingRemote<brave_wallet::mojom::JsonRpcServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  base::OnceClosure callback_;
  std::string expected_chain_id_;
  brave_wallet::mojom::CoinType expected_coin_;
  std::string expected_error_;
  bool chain_changed_called_ = false;
  mojo::Receiver<brave_wallet::mojom::JsonRpcServiceObserver>
      observer_receiver_{this};
};

class BraveWalletEthereumChainTest : public InProcessBrowserTest {
 public:
  BraveWalletEthereumChainTest() {
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
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&BraveWalletEthereumChainTest::HandleChainRequest,
                            base::Unretained(this)));

    ASSERT_TRUE(https_server_->Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleChainRequest(
      const net::test_server::HttpRequest& request) {
    GURL absolute_url = https_server_->GetURL(request.relative_url);
    if (absolute_url.path() != "/rpc") {
      return nullptr;
    }
    std::map<std::string, std::string> params;
    ExtractParameters(absolute_url.query(), &params);
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    auto chain_id = params.size() ? params["id"] : kSomeChainId;
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
    if (!query.empty()) {
      rpc_query += "&" + query;
    }
    replacements.SetQueryStr(rpc_query);
    auto url =
        https_server()->GetURL(host, "/brave_wallet_ethereum_chain.html");
    return url.ReplaceComponents(replacements);
  }

  brave_wallet::JsonRpcService* GetJsonRpcService() {
    return brave_wallet::JsonRpcServiceFactory::GetInstance()
        ->GetServiceForContext(browser()->profile());
  }

  std::vector<brave_wallet::mojom::NetworkInfoPtr> GetAllEthCustomChains() {
    return brave_wallet::GetAllCustomChains(browser()->profile()->GetPrefs(),
                                            brave_wallet::mojom::CoinType::ETH);
  }

  void CallAndWaitForEthereumChainRequestCompleted(
      const std::string& chain_id,
      bool approved,
      brave_wallet::mojom::CoinType coin,
      const std::string& error) {
    base::RunLoop loop;
    std::unique_ptr<TestJsonRpcServiceObserver> observer(
        new TestJsonRpcServiceObserver(loop.QuitClosure(), chain_id, coin,
                                       error));
    GetJsonRpcService()->AddObserver(observer->GetReceiver());

    mojo::PendingRemote<brave_wallet::mojom::JsonRpcServiceObserver> receiver;
    mojo::MakeSelfOwnedReceiver(std::move(observer),
                                receiver.InitWithNewPipeAndPassReceiver());

    GetJsonRpcService()->AddEthereumChainRequestCompleted(chain_id, approved);
    loop.Run();
  }

  std::string GetPendingSwitchChainRequestId() {
    auto requests = GetJsonRpcService()->GetPendingSwitchChainRequestsSync();
    EXPECT_EQ(1u, requests.size());
    return requests[0]->request_id;
  }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddEthereumChainApproved) {
  ASSERT_TRUE(GetAllEthCustomChains().empty());
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
  CallAndWaitForEthereumChainRequestCompleted(
      kSomeChainId, true, brave_wallet::mojom::CoinType::ETH, "");
  const url::Origin origin = url::Origin::Create(url);
  GetJsonRpcService()->NotifySwitchChainRequestProcessed(
      GetPendingSwitchChainRequestId(), true);
  auto result_first = EvalJs(contents, kScriptWaitForEvent);
  EXPECT_EQ(base::Value(true), result_first.value);
  ASSERT_FALSE(GetAllEthCustomChains().empty());
  auto chain = GetAllEthCustomChains().front().Clone();
  EXPECT_EQ(chain->chain_id, kSomeChainId);
  EXPECT_EQ(chain->icon_urls,
            std::vector<std::string>(
                {"https://test.com/icon.png", "http://localhost/"}));
  EXPECT_EQ(
      chain->block_explorer_urls,
      std::vector<std::string>({"https://bscscan.com/", "http://localhost/"}));
  EXPECT_EQ(GetJsonRpcService()->GetChainIdSync(
                brave_wallet::mojom::CoinType::ETH, origin),
            kSomeChainId);
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
  GetJsonRpcService()->AddEthereumChainRequestCompleted(kSomeChainId, false);
  auto result_first = EvalJs(contents, kScriptWaitForEvent);
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
  auto result_first = EvalJs(contents, kScriptRunAndCheckAddChainResult);
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
  auto rejected_same_id = EvalJs(web_contentsB, kScriptWaitForEvent);
  EXPECT_EQ(base::Value(false), rejected_same_id.value);
  ASSERT_FALSE(tab_helperB->IsShowingBubble());
  ASSERT_FALSE(tab_helperA->IsShowingBubble());
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest,
                       AddDifferentChainsNoSwitch) {
  ASSERT_TRUE(GetAllEthCustomChains().empty());

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
  CallAndWaitForEthereumChainRequestCompleted(
      "0x11", true, brave_wallet::mojom::CoinType::ETH, "");
  const url::Origin origin = url::Origin::Create(urlB);
  GetJsonRpcService()->NotifySwitchChainRequestProcessed(
      GetPendingSwitchChainRequestId(), false);
  auto rejected_same_id = EvalJs(web_contentsB, kScriptWaitForEvent);
  EXPECT_EQ(base::Value(false), rejected_same_id.value);
  base::RunLoop().RunUntilIdle();
  // Chain should still exist though
  ASSERT_FALSE(GetAllEthCustomChains().empty());
  EXPECT_EQ(GetAllEthCustomChains().front()->chain_id, "0x11");
  // But current chain should not change
  EXPECT_EQ(GetJsonRpcService()->GetChainIdSync(
                brave_wallet::mojom::CoinType::ETH, origin),
            "0x1");
}

IN_PROC_BROWSER_TEST_F(BraveWalletEthereumChainTest, AddDifferentChainsSwitch) {
  ASSERT_TRUE(GetAllEthCustomChains().empty());

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
  CallAndWaitForEthereumChainRequestCompleted(
      "0x11", true, brave_wallet::mojom::CoinType::ETH, "");
  const url::Origin origin = url::Origin::Create(urlB);
  GetJsonRpcService()->NotifySwitchChainRequestProcessed(
      GetPendingSwitchChainRequestId(), true);
  auto rejected_same_id = EvalJs(web_contentsB, kScriptWaitForEvent);
  EXPECT_EQ(base::Value(true), rejected_same_id.value);
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(GetAllEthCustomChains().empty());
  EXPECT_EQ(GetAllEthCustomChains().front()->chain_id, "0x11");
  EXPECT_EQ(GetJsonRpcService()->GetChainIdSync(
                brave_wallet::mojom::CoinType::ETH, origin),
            "0x11");
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
  ASSERT_TRUE(GetAllEthCustomChains().empty());

  CallAndWaitForEthereumChainRequestCompleted(
      "0x11", true, brave_wallet::mojom::CoinType::ETH, "");
  ASSERT_FALSE(GetAllEthCustomChains().empty());
  EXPECT_EQ(GetAllEthCustomChains().front()->chain_id, "0x11");
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
  auto result_first = EvalJs(contents, kScriptRunEmptyAndCheckChainResult);
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
  EXPECT_EQ(content::EvalJs(contents, "document.title;"),
            "PAGE_SCRIPT_STARTED");
  auto result_first = EvalJs(contents, "window.ethereum != null");
  EXPECT_EQ(base::Value(false), result_first.value);
}
