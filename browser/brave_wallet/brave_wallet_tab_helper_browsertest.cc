/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/fake_usb_chooser_controller.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/web_contents_tester.h"
#include "net/dns/mock_host_resolver.h"

namespace {

constexpr char kEmbeddedTestServerDirectory[] = "brave-wallet";

base::OnceClosure ShowChooserBubble(
    content::WebContents* contents,
    std::unique_ptr<permissions::ChooserController> controller) {
  return chrome::ShowDeviceChooserDialog(contents->GetPrimaryMainFrame(),
                                         std::move(controller));
}

void ExecuteScriptToOpenPopup(content::WebContents* web_contents,
                              const GURL& url) {
  content::TestNavigationObserver popup_waiter(nullptr, 1);
  popup_waiter.StartWatchingNewWebContents();
  ASSERT_EQ(true,
            content::EvalJs(web_contents,
                            content::JsReplace("!!window.open($1);", url)));
  popup_waiter.Wait();
}

int32_t OpenNonPanelPopup(const GURL& url,
                          Browser* browser,
                          content::WebContents* web_contents) {
  EXPECT_EQ(1, browser->tab_strip_model()->count());
  content::TestNavigationObserver popup_waiter(nullptr, 1);
  popup_waiter.StartWatchingNewWebContents();
  ExecuteScriptToOpenPopup(web_contents, url);
  popup_waiter.Wait();
  EXPECT_EQ(2, browser->tab_strip_model()->count());
  content::WebContents* popup =
      browser->tab_strip_model()->GetActiveWebContents();
  auto popup_id = sessions::SessionTabHelper::IdForTab(popup).id();
  auto* child_popup = brave_wallet::GetWebContentsFromTabId(nullptr, popup_id);
  EXPECT_EQ(child_popup->GetVisibleURL(), url);
  return popup_id;
}

int32_t OpenPanelPopup(const GURL& url,
                       content::WebContents* panel_contents,
                       brave_wallet::BraveWalletTabHelper* tab_helper) {
  auto current_size = tab_helper->GetPopupIdsForTesting().size();
  ExecuteScriptToOpenPopup(panel_contents, url);
  auto popup_ids = tab_helper->GetPopupIdsForTesting();
  EXPECT_EQ(popup_ids.size(), current_size + 1);
  auto popup_id = popup_ids.back();
  auto* child_popup = brave_wallet::GetWebContentsFromTabId(nullptr, popup_id);
  EXPECT_EQ(child_popup->GetVisibleURL(), url);
  return popup_id;
}

}  // namespace

class BraveWalletTabHelperBrowserTest : public InProcessBrowserTest {
 public:
  BraveWalletTabHelperBrowserTest() {
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
  base::test::ScopedFeatureList feature_list_;
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
  ASSERT_FALSE(tab_helper->CloseOnDeactivateForTesting());
  std::move(close_dialog_callback).Run();
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(tab_helper->CloseOnDeactivateForTesting());
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
  ASSERT_TRUE(tab_helper->CloseOnDeactivateForTesting());
  std::move(close_dialog_callback).Run();
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(tab_helper->CloseOnDeactivateForTesting());
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
  auto non_panel_popup_id =
      OpenNonPanelPopup(blank_url, browser(), active_contents);
  browser()->tab_strip_model()->ActivateTabAt(0);
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active_contents);
  ASSERT_TRUE(
      brave_wallet::GetWebContentsFromTabId(nullptr, non_panel_popup_id));
  tab_helper->ShowApproveWalletBubble();
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  auto* panel_contents = tab_helper->GetBubbleWebContentsForTesting();
  WaitForLoadStop(panel_contents);
  tab_helper->SetCloseOnDeactivate(false);
  auto popup1_id =
      OpenPanelPopup(https_server()->GetURL("a.com", "/popup.html"),
                     panel_contents, tab_helper);
  auto popup2_id =
      OpenPanelPopup(https_server()->GetURL("b.com", "/popup.html"),
                     panel_contents, tab_helper);
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  chrome::NewTab(browser());
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  Browser* target_browser = nullptr;
  ASSERT_FALSE(
      brave_wallet::GetWebContentsFromTabId(&target_browser, popup1_id));
  EXPECT_EQ(target_browser, nullptr);
  target_browser = nullptr;
  ASSERT_FALSE(
      brave_wallet::GetWebContentsFromTabId(&target_browser, popup2_id));
  EXPECT_EQ(target_browser, nullptr);
  EXPECT_EQ(tab_helper->GetPopupIdsForTesting().size(), 0u);
  target_browser = nullptr;
  ASSERT_TRUE(brave_wallet::GetWebContentsFromTabId(&target_browser,
                                                    non_panel_popup_id));
  EXPECT_EQ(target_browser, browser());
}

IN_PROC_BROWSER_TEST_F(BraveWalletTabHelperBrowserTest, ClosePopupsWithBubble) {
  auto blank_url = https_server()->GetURL("c.com", "/popup.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), blank_url));
  content::WebContents* active_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(active_contents);
  auto non_panel_popup_id =
      OpenNonPanelPopup(blank_url, browser(), active_contents);
  browser()->tab_strip_model()->ActivateTabAt(0);
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active_contents);
  Browser* target_browser = nullptr;
  ASSERT_TRUE(brave_wallet::GetWebContentsFromTabId(&target_browser,
                                                    non_panel_popup_id));
  EXPECT_EQ(browser(), target_browser);
  tab_helper->ShowApproveWalletBubble();
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  auto* panel_contents = tab_helper->GetBubbleWebContentsForTesting();
  WaitForLoadStop(panel_contents);
  tab_helper->SetCloseOnDeactivate(false);
  auto popup1_id =
      OpenPanelPopup(https_server()->GetURL("a.com", "/popup.html"),
                     panel_contents, tab_helper);
  auto popup2_id =
      OpenPanelPopup(https_server()->GetURL("b.com", "/popup.html"),
                     panel_contents, tab_helper);
  EXPECT_TRUE(tab_helper->IsShowingBubble());
  tab_helper->CloseBubble();
  base::RunLoop().RunUntilIdle();
  ASSERT_FALSE(tab_helper->IsShowingBubble());
  ASSERT_FALSE(brave_wallet::GetWebContentsFromTabId(nullptr, popup1_id));
  ASSERT_FALSE(brave_wallet::GetWebContentsFromTabId(nullptr, popup2_id));
  EXPECT_EQ(tab_helper->GetPopupIdsForTesting().size(), 0u);
  ASSERT_TRUE(
      brave_wallet::GetWebContentsFromTabId(nullptr, non_panel_popup_id));
}
