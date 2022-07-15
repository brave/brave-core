// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/javascript_dialogs/app_modal_dialog_controller.h"
#include "components/javascript_dialogs/app_modal_dialog_view.h"
#include "components/javascript_dialogs/tab_modal_dialog_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"

using content::EvalJsResult;

namespace {

std::string SelectInNetworkList(const std::string& selector) {
  return base::StringPrintf("window.testing.walletNetworks.querySelector(`%s`)",
                            selector.c_str());
}

std::string SelectInAddNetworkDialog(const std::string& selector) {
  return base::StringPrintf(
      "window.testing.addWalletNetworkDialog.querySelector(`%s`)",
      selector.c_str());
}

std::string DoubleClickOn(const std::string& element) {
  return element +
         ".dispatchEvent((function (){const e = "
         "document.createEvent('MouseEvents');e.initEvent('dblclick',true,true)"
         ";return e;})())";
}

std::string CeloNetwork() {
  return "[data-test-chain-id='chain-0xa4ec']";
}

std::string PolygonNetwork() {
  return "[data-test-chain-id='chain-0x89']";
}

std::string NetworkNameSpan() {
  return "[class|='NetworkName']";
}

std::string CeloNetworkHideButton() {
  return CeloNetwork() + " .hide-network-button";
}

std::string CeloNetworkChainName() {
  return CeloNetwork() + " .chainName";
}

std::string NetworksButton() {
  return R"([data-test-id='select-network-button'])";
}

std::string QuerySelectorJS(const std::string& selector) {
  return base::StringPrintf(R"(document.querySelector(`%s`))",
                            selector.c_str());
}

std::string Select(const std::string& selector1, const std::string& selector2) {
  return base::StringPrintf(R"(document.querySelector(`%s %s`))",
                            selector1.c_str(), selector2.c_str());
}

void NonBlockingDelay(const base::TimeDelta& delay) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
  run_loop.Run();
}

bool WaitFor(content::WebContents* web_contents, const std::string& selector) {
  for (int i = 0; i < 1000; ++i) {
    if (EvalJs(web_contents, "!!(" + selector + ")").ExtractBool())
      return true;
    NonBlockingDelay(base::Milliseconds(10));
  }
  return false;
}

bool WaitAndClickElement(content::WebContents* web_contents,
                         const std::string& selector) {
  if (!WaitFor(web_contents, selector))
    return false;

  return EvalJs(web_contents, selector + ".click()").value.is_none();
}

bool ClickElementAsync(content::WebContents* web_contents,
                       const std::string& selector) {
  auto script =
      base::StringPrintf("setTimeout(() =>{%s.click()}, 0)", selector.c_str());
  return EvalJs(web_contents, script).value.is_none();
}

}  // namespace

class WalletPanelUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Disabling CSP on webui pages so EvalJS could be run in main world.
    BraveSettingsUI::ShouldDisableCSPForTesting() = true;
    BraveSettingsUI::ShouldExposeElementsForTesting() = true;
    WalletPanelUI::ShouldDisableCSPForTesting() = true;

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    brave_wallet::JsonRpcServiceFactory::GetServiceForContext(
        browser()->profile())
        ->SetAPIRequestHelperForTesting(shared_url_loader_factory_);

    brave_wallet::KeyringServiceFactory::GetServiceForContext(
        browser()->profile())
        ->CreateWallet("password_123", base::DoNothing());

    SetEthChainIdInterceptor(
        brave_wallet::GetKnownEthChain(browser()->profile()->GetPrefs(),
                                       brave_wallet::mojom::kCeloMainnetChainId)
            ->rpc_urls.front(),
        brave_wallet::mojom::kCeloMainnetChainId);

    CreateWalletTab();
  }

  void CreateWalletTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), GURL(kBraveUIWalletPanelURL),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    wallet_ = browser()->tab_strip_model()->GetActiveWebContents();
  }

  void CreateSettingsTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), GURL(std::string(kWalletSettingsURL) + "/networks"),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    settings_ = browser()->tab_strip_model()->GetActiveWebContents();
  }

  void ActivateSettingsTab() {
    browser()->tab_strip_model()->ActivateTabAt(
        browser()->tab_strip_model()->GetIndexOfWebContents(settings()));
  }

  void ActivateWalletTab() {
    browser()->tab_strip_model()->ActivateTabAt(
        browser()->tab_strip_model()->GetIndexOfWebContents(wallet()));
  }

  WalletPanelUI* GetWebUIController() {
    return wallet_->GetWebUI()
        ->GetController()
        ->template GetAs<WalletPanelUI>();
  }

  void SetEthChainIdInterceptor(const std::string& network_url,
                                const std::string& chain_id) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=](const network::ResourceRequest& request) {
          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("eth_chainId") != std::string::npos) {
            url_loader_factory_.AddResponse(
                network_url,
                base::StringPrintf(R"({"jsonrpc":"2.0","id":1,"result":"%s"})",
                                   chain_id.c_str()));
          }
        }));
  }

  content::WebContents* wallet() { return wallet_; }
  content::WebContents* settings() { return settings_; }

 private:
  content::WebContents* wallet_ = nullptr;
  content::WebContents* settings_ = nullptr;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, InitialUIRendered) {
  const std::string wallet_panel_js = "!!document.querySelector('#mountPoint')";
  ASSERT_TRUE(EvalJs(wallet(), wallet_panel_js).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, HideNetworkInSettings) {
  ActivateWalletTab();
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Both Polygon and Celo are listed.
  ASSERT_TRUE(WaitFor(wallet(), QuerySelectorJS(PolygonNetwork())));
  ASSERT_TRUE(
      EvalJs(wallet(), QuerySelectorJS(PolygonNetwork())).value.is_dict());
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(CeloNetwork())).value.is_dict());

  // Wait and click on hide button for Celo network in settings.
  CreateSettingsTab();
  ActivateSettingsTab();
  ASSERT_TRUE(WaitAndClickElement(
      settings(), SelectInNetworkList(CeloNetworkHideButton())));

  ActivateWalletTab();
  wallet()->GetController().Reload(content::ReloadType::NORMAL, true);
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Polygon is listed but Celo is not.
  ASSERT_TRUE(WaitFor(wallet(), QuerySelectorJS(PolygonNetwork())));
  ASSERT_TRUE(
      EvalJs(wallet(), QuerySelectorJS(PolygonNetwork())).value.is_dict());
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(CeloNetwork())).value.is_none());
}

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, CustomNetworkInSettings) {
  CreateSettingsTab();

  ActivateWalletTab();
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Celo Mainnet is listed in wallet.
  ASSERT_TRUE(WaitFor(wallet(), Select(CeloNetwork(), NetworkNameSpan()) +
                                    "?.innerText === 'Celo Mainnet'"));

  // Go to wallet network settings and wait for Celo network to appear.
  ActivateSettingsTab();
  ASSERT_TRUE(WaitFor(settings(), SelectInNetworkList(CeloNetworkChainName()) +
                                      "?.innerText === 'Celo Mainnet'"));

  // Double click on Celo network.
  ASSERT_TRUE(
      EvalJs(settings(), DoubleClickOn(SelectInNetworkList(CeloNetwork())))
          .ExtractBool());

  // Wait for edit network dialog with Celo Mainnet as chain name.
  ASSERT_TRUE(WaitFor(settings(), SelectInAddNetworkDialog("#chainName") +
                                      "?.value === 'Celo Mainnet'"));

  // Change name to 'Custom Network'.
  ASSERT_EQ("Custom Network",
            EvalJs(settings(), SelectInAddNetworkDialog("#chainName") +
                                   ".value = 'Custom Network'")
                .ExtractString());

  // Click on submit button.
  // TODO(apaymyshev): should do js confirm dialog instead of native one:
  // https://github.com/brave/brave-browser/issues/23472
  auto* js_dialog_manager =
      javascript_dialogs::TabModalDialogManager::FromWebContents(settings());
  base::RunLoop dialog_wait;
  js_dialog_manager->SetDialogShownCallbackForTesting(
      dialog_wait.QuitClosure());

  ClickElementAsync(settings(),
                    SelectInAddNetworkDialog("cr-button.action-button"));
  dialog_wait.Run();
  js_dialog_manager->ClickDialogButtonForTesting(true, {});
  web_modal::WebContentsModalDialogManager::FromWebContents(settings())
      ->CloseAllDialogs();

  // Chain name for Celo changes to 'Custom Network' in settings.
  ASSERT_TRUE(WaitFor(settings(), SelectInNetworkList(CeloNetworkChainName()) +
                                      "?.innerText === 'Custom Network'"));

  // Chain name for Celo changes to 'Custom Network' in wallet.
  ActivateWalletTab();
  ASSERT_TRUE(WaitFor(wallet(), Select(CeloNetwork(), NetworkNameSpan()) +
                                    "?.innerText === 'Custom Network'"));
}
