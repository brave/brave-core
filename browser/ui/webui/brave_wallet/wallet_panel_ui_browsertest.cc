// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"

#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using content::EvalJsResult;

namespace {

constexpr char kSomeEndpoint[] = "https://some.endpoint.com/";

std::string SelectInNetworkList(const std::string& selector) {
  return absl::StrFormat("window.testing.walletNetworks60.querySelector(`%s`)",
                         selector);
}

std::string SelectInAddNetworkDialog(const std::string& selector) {
  return absl::StrFormat(
      "window.testing.addWalletNetworkDialog.querySelector(`%s`)", selector);
}

std::string DoubleClickOn(const std::string& element) {
  return element +
         ".dispatchEvent((function (){const e = "
         "document.createEvent('MouseEvents');e.initEvent('dblclick',true,true)"
         ";return e;})())";
}

std::string NeonEVMNetwork() {
  return "[data-test-chain-id='chain-0xe9ac0d6']";
}

std::string PolygonNetwork() {
  return "[data-test-chain-id='chain-0x89']";
}

std::string NetworkNameSpan() {
  return "[class|='NetworkName']";
}

std::string NeonEVMNetworkHideButton() {
  return NeonEVMNetwork() + " .hide-network-button";
}

std::string NeonEVMNetworkChainName() {
  return NeonEVMNetwork() + " .chainName";
}

std::string NetworksButton() {
  return R"([data-test-id='select-network-button'])";
}

std::string QuerySelectorJS(const std::string& selector) {
  return absl::StrFormat(R"(document.querySelector(`%s`))", selector);
}

std::string Select(const std::string& selector1, const std::string& selector2) {
  return absl::StrFormat(R"(document.querySelector(`%s %s`))", selector1,
                         selector2);
}

void NonBlockingDelay(base::TimeDelta delay) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
  run_loop.Run();
}

bool WaitFor(content::WebContents* web_contents, const std::string& selector) {
  for (int i = 0; i < 1000; ++i) {
    if (EvalJs(web_contents, "!!(" + selector + ")").ExtractBool()) {
      return true;
    }
    NonBlockingDelay(base::Milliseconds(10));
  }
  return false;
}

bool WaitAndClickElement(content::WebContents* web_contents,
                         const std::string& selector) {
  for (int i = 0; i < 10; ++i) {
    if (!WaitFor(web_contents, selector)) {
      return false;
    }
    auto result = EvalJs(web_contents, selector + ".click()");
    if (result.is_ok() && result == base::Value()) {
      return true;
    }
  }
  return false;
}

}  // namespace

namespace brave_wallet {

class WalletPanelUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    BraveSettingsUI::ShouldExposeElementsForTesting() = true;

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);

    brave_wallet_service()->json_rpc_service()->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);

    AssetRatioServiceFactory::GetServiceForContext(browser()->profile())
        ->EnableDummyPricesForTesting();

    brave_wallet_service()->keyring_service()->CreateWallet("password_123",
                                                            base::DoNothing());

    SetEthChainIdInterceptor(
        {GURL(kSomeEndpoint), brave_wallet_service()
                                  ->network_manager()
                                  ->GetKnownChain(mojom::kNeonEVMMainnetChainId,
                                                  mojom::CoinType::ETH)
                                  ->rpc_endpoints.front()},
        mojom::kNeonEVMMainnetChainId);

    CreateWalletTab();
  }

  void CreateWalletTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(),
        GURL(std::string(kBraveUIWalletPanelURL) + "crypto/connections"),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    wallet_index_ = browser()->tab_strip_model()->active_index();
  }

  void CreateSettingsTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), GURL(std::string(kWalletSettingsURL) + "/networks"),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    settings_index_ = browser()->tab_strip_model()->active_index();
    // Overriding native confirmation dialog so it always confirms.
    EXPECT_TRUE(EvalJs(settings(), "window.confirm = () => true").is_ok());
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
    return wallet()->GetWebUI()->GetController()->GetAs<WalletPanelUI>();
  }

  void SetEthChainIdInterceptor(const std::vector<GURL>& network_urls,
                                const std::string& chain_id) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=, this](const network::ResourceRequest& request) {
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("eth_chainId") != std::string::npos) {
            const std::string response = absl::StrFormat(
                R"({"jsonrpc":"2.0","id":1,"result":"%s"})", chain_id);
            for (auto& url : network_urls) {
              url_loader_factory_.AddResponse(url.spec(), response);
            }
          }
        }));
  }

  void WaitForNeonEVMNetworkUrl(const GURL& url) {
    auto* prefs = browser()->profile()->GetPrefs();

    if (brave_wallet_service()->network_manager()->GetNetworkURL(
            mojom::kNeonEVMMainnetChainId, mojom::CoinType::ETH) == url) {
      return;
    }

    base::RunLoop run_loop;
    PrefChangeRegistrar pref_change_registrar;
    pref_change_registrar.Init(prefs);
    pref_change_registrar.Add(
        kBraveWalletCustomNetworks, base::BindLambdaForTesting([&] {
          if (brave_wallet_service()->network_manager()->GetNetworkURL(
                  mojom::kNeonEVMMainnetChainId, mojom::CoinType::ETH) == url) {
            run_loop.Quit();
          }
        }));
    run_loop.Run();
  }

  content::WebContents* wallet() {
    return browser()->tab_strip_model()->GetWebContentsAt(wallet_index_);
  }
  content::WebContents* settings() {
    return browser()->tab_strip_model()->GetWebContentsAt(settings_index_);
  }

  BraveWalletService* brave_wallet_service() {
    return BraveWalletServiceFactory::GetServiceForContext(
        browser()->profile());
  }

 private:
  int wallet_index_ = 0;
  int settings_index_ = 0;
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

  // Both Polygon and Neon EVM are listed.
  ASSERT_TRUE(WaitFor(wallet(), QuerySelectorJS(PolygonNetwork())));
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(PolygonNetwork())).is_dict());
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(NeonEVMNetwork())).is_dict());

  // Wait and click on hide button for Neon EVM network in settings.
  CreateSettingsTab();
  ActivateSettingsTab();
  ASSERT_TRUE(WaitAndClickElement(
      settings(), SelectInNetworkList(NeonEVMNetworkHideButton())));

  ActivateWalletTab();
  wallet()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(wallet()));
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Polygon is listed but Neon EVM is not.
  ASSERT_TRUE(WaitFor(wallet(), QuerySelectorJS(PolygonNetwork())));
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(PolygonNetwork())).is_dict());
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(NeonEVMNetwork())).is_ok());
}

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, CustomNetworkInSettings) {
  CreateSettingsTab();

  ActivateWalletTab();
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Neon EVM is listed in wallet.
  ASSERT_TRUE(WaitFor(wallet(), Select(NeonEVMNetwork(), NetworkNameSpan()) +
                                    "?.innerText === 'Neon EVM'"));

  // Go to wallet network settings and wait for Neon EVM network to appear.
  ActivateSettingsTab();
  ASSERT_TRUE(
      WaitFor(settings(), SelectInNetworkList(NeonEVMNetworkChainName()) +
                              "?.innerText === 'Neon EVM'"));

  // Double-click on Neon EVM network.
  ASSERT_TRUE(
      EvalJs(settings(), DoubleClickOn(SelectInNetworkList(NeonEVMNetwork())))
          .ExtractBool());

  // Wait for edit network dialog with Neon EVM as chain name.
  ASSERT_TRUE(WaitFor(settings(), SelectInAddNetworkDialog("#chainName") +
                                      "?.value === 'Neon EVM'"));

  // Change name to 'Custom Network'.
  ASSERT_EQ("Custom Network",
            EvalJs(settings(), SelectInAddNetworkDialog("#chainName") +
                                   ".value = 'Custom Network'")
                .ExtractString());

  // Click on submit button.
  ASSERT_TRUE(WaitAndClickElement(settings(),
                                  SelectInAddNetworkDialog(".action-button")));

  // Chain name for Neon EVM changes to 'Custom Network' in settings.
  ASSERT_TRUE(
      WaitFor(settings(), SelectInNetworkList(NeonEVMNetworkChainName()) +
                              "?.innerText === 'Custom Network'"));

  // Chain name for Neon EVM changes to 'Custom Network' in wallet.
  ActivateWalletTab();
  wallet()->GetController().Reload(content::ReloadType::NORMAL, true);
  EXPECT_TRUE(WaitForLoadStop(wallet()));
  ASSERT_TRUE(WaitFor(wallet(), Select(NeonEVMNetwork(), NetworkNameSpan()) +
                                    "?.innerText === 'Custom Network'"));
}

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, SelectRpcEndpoint) {
  CreateSettingsTab();
  auto known_neon_evm_rpc =
      brave_wallet_service()
          ->network_manager()
          ->GetKnownChain(mojom::kNeonEVMMainnetChainId, mojom::CoinType::ETH)
          ->rpc_endpoints.front();
  // Neon EVM rpc is from known info.
  WaitForNeonEVMNetworkUrl(known_neon_evm_rpc);

  // Go to wallet network settings and wait for Neon EVM network to appear.
  ActivateSettingsTab();
  ASSERT_TRUE(
      WaitFor(settings(), SelectInNetworkList(NeonEVMNetworkChainName()) +
                              "?.innerText === 'Neon EVM'"));

  // Double-click on Neon EVM network.
  ASSERT_TRUE(
      EvalJs(settings(), DoubleClickOn(SelectInNetworkList(NeonEVMNetwork())))
          .ExtractBool());

  // Wait for edit network dialog with Neon EVM as chain name.
  ASSERT_TRUE(WaitFor(settings(), SelectInAddNetworkDialog("#chainName") +
                                      "?.value === 'Neon EVM'"));

  // Click rpc + button.
  ASSERT_TRUE(WaitAndClickElement(
      settings(), SelectInAddNetworkDialog("#rpc-plus-button")));

  // Click on added input.
  ASSERT_TRUE(WaitAndClickElement(
      settings(),
      SelectInAddNetworkDialog(
          "#rpcRadioGroup cr-radio-button:nth-of-type(1) cr-input")));

  // Set value to added input.
  ASSERT_TRUE(
      WaitFor(settings(),
              SelectInAddNetworkDialog(
                  "#rpcRadioGroup cr-radio-button:nth-of-type(1) cr-input") +
                  ".value='" + kSomeEndpoint + "'"));

  // Submit changes.
  ASSERT_TRUE(WaitAndClickElement(
      settings(), SelectInAddNetworkDialog("cr-button.action-button")));

  // Wait for custom endpoint for Neon EVM.
  WaitForNeonEVMNetworkUrl(GURL(kSomeEndpoint));

  // Wait for custom endpoint listed for Neon EVM.
  ASSERT_TRUE(
      WaitFor(settings(),
              SelectInNetworkList(NeonEVMNetwork() + " .secondary") +
                  "?.innerText === '0xe9ac0d6 https://some.endpoint.com/'"));

  // Double-click on Neon EVM network.
  ASSERT_TRUE(
      EvalJs(settings(), DoubleClickOn(SelectInNetworkList(NeonEVMNetwork())))
          .ExtractBool());
  // Wait for edit network dialog with Neon EVM as chain name.
  ASSERT_TRUE(WaitFor(settings(), SelectInAddNetworkDialog("#chainName") +
                                      "?.value === 'Neon EVM'"));
  // Click on second item(known rpc) in rpc list.
  ASSERT_TRUE(WaitAndClickElement(
      settings(),
      SelectInAddNetworkDialog(
          "#rpcRadioGroup cr-radio-button:nth-of-type(2) cr-input")));
  // Sumbit changes.
  ASSERT_TRUE(WaitAndClickElement(
      settings(), SelectInAddNetworkDialog("cr-button.action-button")));

  // Wait for endpoint to become known one.
  WaitForNeonEVMNetworkUrl(known_neon_evm_rpc);
}

}  // namespace brave_wallet
