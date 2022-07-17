/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
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

class JSEthereumProviderBrowserTest : public InProcessBrowserTest {
 public:
  JSEthereumProviderBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
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

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetMainFrame();
  }

  void ReloadAndWaitForLoadStop() {
    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  }

 protected:
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, AttachOnReload) {
  brave_wallet::SetDefaultWallet(browser()->profile()->GetPrefs(),
                                 brave_wallet::mojom::DefaultWallet::None);
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::string command = "window.ethereum.isMetaMask";
  EXPECT_TRUE(content::EvalJs(main_frame(), command)
                  .error.find("Cannot read properties of undefined") !=
              std::string::npos);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  brave_wallet::SetDefaultWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  ReloadAndWaitForLoadStop();
  auto result = content::EvalJs(main_frame(), command);
  EXPECT_EQ(result.error, "");
  ASSERT_TRUE(result.ExtractBool());
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  // unable to overwrite
  std::string overwrite = "window.ethereum = ['test'];window.ethereum[0]";
  EXPECT_EQ(content::EvalJs(main_frame(), overwrite).error, "");
  ASSERT_TRUE(content::EvalJs(main_frame(), command).ExtractBool());
  brave_wallet::SetDefaultWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  ReloadAndWaitForLoadStop();
  // overwrite successfully
  EXPECT_EQ(content::EvalJs(main_frame(), overwrite).ExtractString(), "test");
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest,
                       DoNotAttachToChromePages) {
  brave_wallet::SetDefaultWallet(browser()->profile()->GetPrefs(),
                                 brave_wallet::mojom::DefaultWallet::None);
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab/")));

  std::string command = "window.ethereum.isMetaMask";
  EXPECT_TRUE(content::EvalJs(main_frame(), command,
                              content::EXECUTE_SCRIPT_DEFAULT_OPTIONS,
                              ISOLATED_WORLD_ID_TRANSLATE)
                  .error.find("Cannot read properties of undefined") !=
              std::string::npos);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  brave_wallet::SetDefaultWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  ReloadAndWaitForLoadStop();
  EXPECT_TRUE(content::EvalJs(main_frame(), command,
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
  for (const std::string& property :
       {"_metamask", "chainId", "networkVersion", "selectedAddress"}) {
    SCOPED_TRACE(property);
    auto result = EvalJs(web_contents(), NonWriteableScriptProperty(property),
                         content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
    EXPECT_EQ(base::Value(true), result.value) << result.error;
  }

  // window.ethereum.* (methods)
  for (const std::string& method :
       {"on", "emit", "removeListener", "removeAllListeners", "request",
        "isConnected", "enable", "sendAsync", "send"}) {
    SCOPED_TRACE(method);
    auto result =
        EvalJs(web_contents(), NonWriteableScriptMethod("ethereum", method),
               content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
    EXPECT_EQ(base::Value(true), result.value) << result.error;
  }
  // window._metamask.isUnlocked()
  auto result =
      EvalJs(web_contents(),
             NonWriteableScriptMethod("ethereum._metamask", "isUnlocked"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value) << result.error;
}

// See https://github.com/brave/brave-browser/issues/22213 for details
IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, IsMetaMaskWritable) {
  const GURL url = https_server_.GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  std::string overwrite =
      "window.ethereum.isMetaMask = false;"
      "window.ethereum.isMetaMask";
  EXPECT_FALSE(content::EvalJs(main_frame(), overwrite).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, NonConfigurable) {
  brave_wallet::SetDefaultWallet(
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
  EXPECT_TRUE(content::EvalJs(main_frame(), overwrite).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, Block3PIframe) {
  GURL top_url(https_server_.GetURL("a.com", "/iframe.html"));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), top_url));
  // third party
  GURL iframe_url_3p(https_server_.GetURL("b.com", "/simple.html"));
  EXPECT_TRUE(NavigateIframeToURL(web_contents(), "test", iframe_url_3p));

  constexpr char kEvalEthereum[] = R"(typeof window.ethereum === 'undefined')";

  auto* iframe_rfh = ChildFrameAt(main_frame(), 0);
  ASSERT_TRUE(iframe_rfh);
  EXPECT_TRUE(content::EvalJs(iframe_rfh, kEvalEthereum).ExtractBool());

  // same party
  GURL iframe_url_1p(https_server_.GetURL("a.com", "/"));
  EXPECT_TRUE(NavigateIframeToURL(web_contents(), "test", iframe_url_1p));
  iframe_rfh = ChildFrameAt(main_frame(), 0);
  ASSERT_TRUE(iframe_rfh);
  EXPECT_FALSE(content::EvalJs(iframe_rfh, kEvalEthereum).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, SecureContextOnly) {
  // Secure context HTTPS server
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  constexpr char kEvalEthereum[] = "typeof window.ethereum !== 'undefined'";
  EXPECT_TRUE(content::EvalJs(main_frame(), kEvalEthereum).ExtractBool());

  // Insecure context
  url = embedded_test_server()->GetURL("a.com", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_FALSE(content::EvalJs(main_frame(), kEvalEthereum).ExtractBool());

  // Secure context localhost HTTP
  url = embedded_test_server()->GetURL("localhost", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(content::EvalJs(main_frame(), kEvalEthereum).ExtractBool());

  // Secure context 127.0.0.1 HTTP
  url = embedded_test_server()->GetURL("localhost", "/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(content::EvalJs(main_frame(), kEvalEthereum).ExtractBool());
}
