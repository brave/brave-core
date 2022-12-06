/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace {
std::string NonWriteableScriptProperty(const std::string& property) {
  return base::StringPrintf(
      R"(window.ethereum.%s = "brave"
         if (window.ethereum.%s === "brave")
           window.domAutomationController.send(false)
         else
           window.domAutomationController.send(true))",
      property.c_str(), property.c_str());
}
std::string NonWriteableScriptMethod(const std::string& provider,
                                     const std::string& method) {
  return base::StringPrintf(
      R"(window.%s.%s = "brave"
         if (typeof window.%s.%s === "function")
           window.domAutomationController.send(true)
         else
           window.domAutomationController.send(false))",
      provider.c_str(), method.c_str(), provider.c_str(), method.c_str());
}
}  // namespace

// TODO(darkdh): Move this browser test to //brave/browser/brave_wallet/ because
// it has layer violation (//chrome/browser,
// //brave/components/brave_wallet/browser and //brave/browser)
class JSEthereumProviderBrowserTest : public InProcessBrowserTest {
 public:
  JSEthereumProviderBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

  ~JSEthereumProviderBrowserTest() override = default;

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
    InProcessBrowserTest::SetUpOnMainThread();

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* primary_main_frame() {
    return web_contents()->GetPrimaryMainFrame();
  }

  void ReloadAndWaitForLoadStop() {
    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  }

  brave_wallet::JsonRpcService* GetJsonRpcService() {
    return brave_wallet::JsonRpcServiceFactory::GetInstance()
        ->GetServiceForContext(browser()->profile());
  }

 protected:
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, AttachOnReload) {
  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::None);
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::string command = "window.ethereum.isMetaMask";
  EXPECT_TRUE(content::EvalJs(primary_main_frame(), command)
                  .error.find("Cannot read properties of undefined") !=
              std::string::npos);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);

  histogram_tester_->ExpectBucketCount("Brave.Wallet.EthProvider.2", 0, 1);

  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  ReloadAndWaitForLoadStop();

  histogram_tester_->ExpectBucketCount("Brave.Wallet.EthProvider.2", 0, 2);

  auto result = content::EvalJs(primary_main_frame(), command);
  EXPECT_EQ(result.error, "");
  ASSERT_TRUE(result.ExtractBool());
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  // unable to overwrite
  std::string overwrite = "window.ethereum = ['test'];window.ethereum[0]";
  EXPECT_EQ(content::EvalJs(primary_main_frame(), overwrite).error, "");
  ASSERT_TRUE(content::EvalJs(primary_main_frame(), command).ExtractBool());
  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  ReloadAndWaitForLoadStop();
  // overwrite successfully
  EXPECT_EQ(content::EvalJs(primary_main_frame(), overwrite).ExtractString(),
            "test");
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest,
                       DoNotAttachToChromePages) {
  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::None);
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab/")));

  std::string command = "window.ethereum.isMetaMask";
  EXPECT_TRUE(content::EvalJs(primary_main_frame(), command,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_TRANSLATE)
                  .error.find("Cannot read properties of undefined") !=
              std::string::npos);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  ReloadAndWaitForLoadStop();

  histogram_tester_->ExpectTotalCount("Brave.Wallet.EthProvider", 0);

  EXPECT_TRUE(content::EvalJs(primary_main_frame(), command,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_TRANSLATE)
                  .error.find("Cannot read properties of undefined") !=
              std::string::npos);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, NonWritable) {
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  // window.ethereum.* (properties)
  for (const std::string& property : {"isBraveWallet", "_metamask", "chainId",
                                      "networkVersion", "selectedAddress"}) {
    SCOPED_TRACE(property);
    auto result = EvalJs(web_contents(), NonWriteableScriptProperty(property),
                         content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
    EXPECT_EQ(base::Value(true), result.value) << result.error;
  }
  // window.ethereum.* (methods)
  // send should be writable because of
  // https://github.com/brave/brave-browser/issues/25078
  for (const std::string& method :
       {"on", "emit", "removeListener", "removeAllListeners", "request",
        "isConnected", "enable", "sendAsync"}) {
    SCOPED_TRACE(method);
    auto result =
        EvalJs(web_contents(), NonWriteableScriptMethod("ethereum", method),
               content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
    EXPECT_EQ(base::Value(true), result.value) << result.error;
  }
  {
    auto result =
        EvalJs(web_contents(), NonWriteableScriptMethod("ethereum", "send"),
               content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
    EXPECT_EQ(base::Value(false), result.value) << result.error;
  }

  // window._metamask.isUnlocked()
  {
    auto result =
        EvalJs(web_contents(),
               NonWriteableScriptMethod("ethereum._metamask", "isUnlocked"),
               content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
    EXPECT_EQ(base::Value(true), result.value) << result.error;
  }
}

// See https://github.com/brave/brave-browser/issues/22213 for details
IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, IsMetaMaskWritable) {
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::string overwrite =
      "window.ethereum.isMetaMask = false;"
      "window.ethereum.isMetaMask";
  EXPECT_FALSE(content::EvalJs(primary_main_frame(), overwrite).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, NonConfigurable) {
  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  std::string overwrite =
      R"(try {
         Object.defineProperty(window, 'ethereum', {
           writable: true,
         });
       } catch (e) {}
       window.ethereum = 42;
       typeof window.ethereum === 'object'
    )";
  EXPECT_TRUE(content::EvalJs(primary_main_frame(), overwrite).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, OnlyWriteOwnProperty) {
  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  const std::string get_chain_id = "window.ethereum.chainId";

  ASSERT_EQ(content::EvalJs(primary_main_frame(), get_chain_id).ExtractString(),
            "0x1");

  GetJsonRpcService()->SetNetwork("0x5", brave_wallet::mojom::CoinType::ETH,
                                  false);
  // Needed so ChainChangedEvent observers run
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(content::EvalJs(primary_main_frame(), get_chain_id).ExtractString(),
            "0x5");

  brave_wallet::SetDefaultEthereumWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  ReloadAndWaitForLoadStop();
  ASSERT_EQ(content::EvalJs(
                primary_main_frame(),
                "window.ethereum = {chainId: '0x89'}; window.ethereum.chainId")
                .ExtractString(),
            "0x89");

  GetJsonRpcService()->SetNetwork("0x4", brave_wallet::mojom::CoinType::ETH,
                                  false);
  // Needed so ChainChangedEvent observers run
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(content::EvalJs(primary_main_frame(), get_chain_id).ExtractString(),
            "0x89");
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, Iframe3P) {
  constexpr char kEvalEthereumUndefined[] =
      R"(typeof window.ethereum === 'undefined')";

  GURL secure_top_url(https_server_.GetURL("a.com", "/iframe.html"));
  GURL insecure_top_url =
      embedded_test_server()->GetURL("a.com", "/iframe.html");
  GURL data_top_url = GURL(
      "data:text/html;,<html><body><iframe id='test'></iframe></body></html>");
  GURL iframe_url_3p(https_server_.GetURL("b.a.com", "/simple.html"));
  GURL iframe_url_1p(https_server_.GetURL("a.com", "/"));
  GURL data_simple_url = GURL("data:text/html;,<html><body></body></html>");

  const struct {
    std::string script;
    GURL top_url;
    GURL iframe_url;
  } ethereum_undefined_cases[] =
      {{// 3p iframe
        "true", secure_top_url, iframe_url_3p},
       {// 1st party iframe with allow="ethereum 'none'"
        R"(
      document.querySelector('iframe').setAttribute('allow', 'ethereum \'none\'');
      true
      )",
        secure_top_url, iframe_url_1p},
       {// 1st party iframe with sandbox="allow-scripts"
        R"(
      document.querySelector('iframe').removeAttribute('allow');
      document.querySelector('iframe').setAttribute('sandbox', 'allow-scripts');
      true
      )",
        secure_top_url, iframe_url_1p},
       {// 3p iframe with sandbox="allow-scripts allow-same-origin"
        R"(
      document.querySelector('iframe').removeAttribute('allow');
      document.querySelector('iframe')
          .setAttribute('sandbox', 'allow-scripts allow-same-origin');
      true
      )",
        secure_top_url, iframe_url_3p},
       {// 3p iframe with allow="solana"
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe').setAttribute('allow', 'solana');
      true
      )",
        secure_top_url, iframe_url_3p},

       {// 3p iframe with allow="solana; ethereum" but insecure top level
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        insecure_top_url, iframe_url_3p},

       {// 3p iframe with allow="solana; ethereum" but insecure top level (data
        // URI)
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        data_top_url, iframe_url_3p},
       {// 3p iframe with allow="solana; ethereum" but insecure iframe
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        secure_top_url, data_simple_url},
       {// insecure top level and insecure iframe allow="solana; ethereum"
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        data_top_url, data_simple_url}},
    ethereum_defined_cases[] = {
        {// 1st party iframe
         "true", secure_top_url, iframe_url_1p},
        {// 1st party iframe sandbox="allow-scripts allow-same-origin"
         R"(
      document.querySelector('iframe').removeAttribute('allow');
      document.querySelector('iframe')
          .setAttribute('sandbox', 'allow-scripts allow-same-origin');
      true
      )",
         secure_top_url, iframe_url_1p},
        {// 3p iframe with allow="ethereum"
         R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe').setAttribute('allow', 'ethereum');
      true
      )",
         secure_top_url, iframe_url_3p},
        {// 3p iframe with allow="solana; ethereum"
         R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
         secure_top_url, iframe_url_3p},
        {// 3rd party iframe with sandbox="allow-scripts" allow="ethereum"
         R"(
      document.querySelector('iframe').setAttribute('allow', 'ethereum');
      document.querySelector('iframe').setAttribute('sandbox', 'allow-scripts');
      true
      )",
         secure_top_url, iframe_url_3p}};

  for (auto& c : ethereum_undefined_cases) {
    SCOPED_TRACE(testing::Message() << c.script << c.top_url << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    EXPECT_TRUE(content::EvalJs(primary_main_frame(), c.script).ExtractBool());
    EXPECT_TRUE(NavigateIframeToURL(web_contents(), "test", c.iframe_url));
    EXPECT_TRUE(content::EvalJs(ChildFrameAt(primary_main_frame(), 0),
                                kEvalEthereumUndefined)
                    .ExtractBool());
  }
  for (auto& c : ethereum_defined_cases) {
    SCOPED_TRACE(testing::Message() << c.script << c.top_url << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    EXPECT_TRUE(content::EvalJs(primary_main_frame(), c.script).ExtractBool());
    EXPECT_TRUE(NavigateIframeToURL(web_contents(), "test", c.iframe_url));
    EXPECT_FALSE(content::EvalJs(ChildFrameAt(primary_main_frame(), 0),
                                 kEvalEthereumUndefined)
                     .ExtractBool());
  }
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, SecureContextOnly) {
  // Secure context HTTPS server
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  constexpr char kEvalEthereum[] = "typeof window.ethereum !== 'undefined'";
  EXPECT_TRUE(
      content::EvalJs(primary_main_frame(), kEvalEthereum).ExtractBool());

  // Insecure context
  url = embedded_test_server()->GetURL("a.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_FALSE(
      content::EvalJs(primary_main_frame(), kEvalEthereum).ExtractBool());

  // Secure context localhost HTTP
  url = embedded_test_server()->GetURL("localhost", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(
      content::EvalJs(primary_main_frame(), kEvalEthereum).ExtractBool());

  // Secure context 127.0.0.1 HTTP
  url = embedded_test_server()->GetURL("localhost", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(
      content::EvalJs(primary_main_frame(), kEvalEthereum).ExtractBool());
}
