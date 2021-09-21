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
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/fake_usb_chooser_controller.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "net/dns/mock_host_resolver.h"

namespace {

const char kEmbeddedTestServerDirectory[] = "brave-wallet";

base::OnceClosure ShowChooserBubble(
    content::WebContents* contents,
    std::unique_ptr<permissions::ChooserController> controller) {
  return chrome::ShowDeviceChooserDialog(contents->GetMainFrame(),
                                         std::move(controller));
}

}  // namespace

class BraveWalletTabHelperBrowserTest : public InProcessBrowserTest {
 public:
  BraveWalletTabHelperBrowserTest() {
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

 private:
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletTabHelperBrowserTest,
                       DoNotHidePanelIfRequestedHIDPermissions) {
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_ethereum_chain.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contents);
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
  GURL url =
      https_server()->GetURL("a.com", "/brave_wallet_ethereum_chain.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  WaitForLoadStop(contents);

  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(contents);
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
