/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/dialogs/browser_dialogs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/fake_usb_chooser_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/web_contents_tester.h"
#include "net/dns/mock_host_resolver.h"

namespace {

base::OnceClosure ShowChooserBubble(
    content::WebContents* contents,
    std::unique_ptr<permissions::ChooserController> controller) {
  return chrome::ShowDeviceChooserDialog(contents->GetPrimaryMainFrame(),
                                         std::move(controller));
}

base::WeakPtr<content::WebContents> ExecuteWindowOpenScript(
    content::WebContents* web_contents,
    const GURL& url) {
  base::test::TestFuture<content::WebContents*> web_contents_future;
  auto creation_subscription = content::RegisterWebContentsCreationCallback(
      web_contents_future.GetRepeatingCallback());

  EXPECT_TRUE(content::EvalJs(web_contents,
                              content::JsReplace("!!window.open($1);", url))
                  .ExtractBool());

  auto opened_web_contents = web_contents_future.Get();
  content::WaitForLoadStop(opened_web_contents);
  return opened_web_contents->GetWeakPtr();
}

base::WeakPtr<content::WebContents> OpenFromRegularPage(
    const GURL& url,
    Browser* browser,
    content::WebContents* web_contents) {
  EXPECT_EQ(1, browser->tab_strip_model()->count());

  auto opened_web_contents = ExecuteWindowOpenScript(web_contents, url);

  EXPECT_EQ(2, browser->tab_strip_model()->count());

  EXPECT_EQ(opened_web_contents.get(),
            browser->tab_strip_model()->GetActiveWebContents());
  EXPECT_EQ(opened_web_contents->GetLastCommittedURL(), url);

  return opened_web_contents->GetWeakPtr();
}

base::WeakPtr<content::WebContents> OpenFromPanel(
    const GURL& url,
    content::WebContents* panel_contents,
    brave_wallet::BraveWalletTabHelper* tab_helper) {
  ui_test_utils::BrowserCreatedObserver browser_created_observer;

  auto opened_web_contents = ExecuteWindowOpenScript(panel_contents, url);

  // New browser window for popup.
  EXPECT_TRUE(browser_created_observer.Wait());

  EXPECT_EQ(opened_web_contents->GetLastCommittedURL(), url);

  return opened_web_contents;
}

}  // namespace

class BraveWalletTabHelperBrowserTest : public InProcessBrowserTest {
 public:
  BraveWalletTabHelperBrowserTest() = default;

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

    https_server_->ServeFilesFromDirectory(
        brave_wallet::BraveWalletTestDataFolder());
    https_server_->RegisterRequestHandler(base::BindRepeating(
        &BraveWalletTabHelperBrowserTest::HandleChainRequest,
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
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    http_response->set_code(net::HTTP_OK);
    http_response->set_content(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0xabcde\"}");
    return http_response;
  }
  GURL GetWalletEthereumChainPageURL() {
    GURL rpc = https_server()->GetURL("c.com", "/rpc");
    std::string rpc_query("rpc=" + rpc.spec());
    GURL::Replacements replacements;
    replacements.SetQueryStr(rpc_query);
    auto url =
        https_server()->GetURL("a.com", "/brave_wallet_ethereum_chain.html");
    return url.ReplaceComponents(replacements);
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletTabHelperBrowserTest,
                       DoNotHidePanelIfRequestedHIDPermissions) {
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

  ASSERT_TRUE(tab_helper->IsShowingBubble());
  auto* panel_contents = tab_helper->GetBubbleWebContentsForTesting();
  auto close_dialog_callback = ShowChooserBubble(
      panel_contents, std::make_unique<FakeUsbChooserController>(1));
  ASSERT_TRUE(tab_helper->IsShowingBubble());
  std::move(close_dialog_callback).Run();
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(tab_helper->IsShowingBubble());
  chrome::NewTab(browser());
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());
}

IN_PROC_BROWSER_TEST_F(BraveWalletTabHelperBrowserTest,
                       HidePanelWhenOthersRequestedHIDPermissions) {
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

  ASSERT_TRUE(tab_helper->IsShowingBubble());
  auto close_dialog_callback = ShowChooserBubble(
      contents, std::make_unique<FakeUsbChooserController>(1));
  std::move(close_dialog_callback).Run();
  base::RunLoop().RunUntilIdle();
  chrome::NewTab(browser());
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());
}

IN_PROC_BROWSER_TEST_F(BraveWalletTabHelperBrowserTest,
                       ClosePopupsWhenSwitchTabs) {
  auto blank_url = https_server()->GetURL("c.com", "/popup.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), blank_url));
  content::WebContents* active_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(active_contents);
  auto non_panel_web_contents =
      OpenFromRegularPage(blank_url, browser(), active_contents);
  browser()->tab_strip_model()->ActivateTabAt(0);
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active_contents);
  ASSERT_TRUE(non_panel_web_contents);
  tab_helper->ShowApproveWalletBubble();
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  auto* panel_contents = tab_helper->GetBubbleWebContentsForTesting();
  WaitForLoadStop(panel_contents);
  auto popup1 = OpenFromPanel(https_server()->GetURL("a.com", "/popup.html"),
                              panel_contents, tab_helper);
  auto popup2 = OpenFromPanel(https_server()->GetURL("b.com", "/popup.html"),
                              panel_contents, tab_helper);
  EXPECT_TRUE(popup1);
  EXPECT_TRUE(popup2);
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  chrome::NewTab(browser());
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());

  EXPECT_FALSE(popup1);
  EXPECT_FALSE(popup2);
  ASSERT_TRUE(non_panel_web_contents);
}

IN_PROC_BROWSER_TEST_F(BraveWalletTabHelperBrowserTest, ClosePopupsWithBubble) {
  auto blank_url = https_server()->GetURL("c.com", "/popup.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), blank_url));
  content::WebContents* active_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(active_contents);
  auto non_panel_web_contents =
      OpenFromRegularPage(blank_url, browser(), active_contents);
  browser()->tab_strip_model()->ActivateTabAt(0);
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active_contents);
  ASSERT_TRUE(non_panel_web_contents);
  tab_helper->ShowApproveWalletBubble();
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  auto* panel_contents = tab_helper->GetBubbleWebContentsForTesting();
  WaitForLoadStop(panel_contents);
  auto popup1 = OpenFromPanel(https_server()->GetURL("a.com", "/popup.html"),
                              panel_contents, tab_helper);
  auto popup2 = OpenFromPanel(https_server()->GetURL("b.com", "/popup.html"),
                              panel_contents, tab_helper);
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  tab_helper->CloseBubble();
  base::RunLoop().RunUntilIdle();

  ASSERT_FALSE(tab_helper->IsShowingBubble());
  ASSERT_FALSE(popup1);
  ASSERT_FALSE(popup2);
  ASSERT_TRUE(non_panel_web_contents);
}
