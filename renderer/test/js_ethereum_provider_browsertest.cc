/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
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
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace {
std::string NonWriteableScript(const std::string& method,
                               const std::string& args) {
  return base::StringPrintf(
      R"(window.ethereum.%s = () => { return "brave" }
         if (window.ethereum.%s%s === "brave")
           window.domAutomationController.send(false)
         else
           window.domAutomationController.send(true))",
      method.c_str(), method.c_str(), args.c_str());
}
}  // namespace

class JSEthereumProviderBrowserTest : public InProcessBrowserTest {
 public:
  JSEthereumProviderBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_.ServeFilesFromDirectory(test_data_dir);
  }

  ~JSEthereumProviderBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    EXPECT_TRUE(https_server_.Start());
    // Map all hosts to localhost.
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetMainFrame();
  }

  void NavigateToURLAndWaitForLoadStop(const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url)) << ":" << url;
    ASSERT_TRUE(WaitForLoadStop(web_contents())) << ":" << url;
  }

  void ReloadAndWaitForLoadStop() {
    chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, AttachOnReload) {
  brave_wallet::SetDefaultWallet(browser()->profile()->GetPrefs(),
                                 brave_wallet::mojom::DefaultWallet::None);
  const GURL url = https_server_.GetURL("/simple.html");
  NavigateToURLAndWaitForLoadStop(url);

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
  NavigateToURLAndWaitForLoadStop(GURL("chrome://newtab/"));

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
  NavigateToURLAndWaitForLoadStop(url);

  // window.ethereum.*
  auto result =
      EvalJs(web_contents(), NonWriteableScript("on", R"(('connect', ()=>{}))"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  LOG(ERROR) << result.error;
  EXPECT_EQ(base::Value(true), result.value);

  auto result2 =
      EvalJs(web_contents(), NonWriteableScript("emit", R"(('connect'))"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);

  auto result3 =
      EvalJs(web_contents(),
             NonWriteableScript("removeListener", R"(('connect', ()=>{}))"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result3.value);

  auto result4 =
      EvalJs(web_contents(), NonWriteableScript("removeAllListeners", R"(())"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result4.value);
}

// See https://github.com/brave/brave-browser/issues/22213 for details
IN_PROC_BROWSER_TEST_F(JSEthereumProviderBrowserTest, IsMetaMaskWritable) {
  const GURL url = https_server_.GetURL("/simple.html");
  NavigateToURLAndWaitForLoadStop(url);

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
  NavigateToURLAndWaitForLoadStop(url);
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
