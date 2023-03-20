// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <string>

#include "base/run_loop.h"
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
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"

using content::EvalJsResult;

namespace {

constexpr char kSomeEndpoint[] = "https://some.endpoint.com/";

std::string SelectInNetworkList(const std::string& selector) {
  return base::StringPrintf(
      "window.testing.walletNetworks60.querySelector(`%s`)", selector.c_str());
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

std::string FantomNetwork() {
  return "[data-test-chain-id='chain-0xfa']";
}

std::string PolygonNetwork() {
  return "[data-test-chain-id='chain-0x89']";
}

std::string NetworkNameSpan() {
  return "[class|='NetworkName']";
}

std::string FantomNetworkHideButton() {
  return FantomNetwork() + " .hide-network-button";
}

std::string FantomNetworkChainName() {
  return FantomNetwork() + " .chainName";
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
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
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

}  // namespace

namespace brave_wallet {

class WalletPanelUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Disabling CSP on webui pages so EvalJS could be run in main world.
    BraveSettingsUI::ShouldDisableCSPForTesting() = true;
    BraveSettingsUI::ShouldExposeElementsForTesting() = true;
    WalletPanelUI::ShouldDisableCSPForTesting() = true;

    auto* profile = browser()->profile();

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    JsonRpcServiceFactory::GetServiceForContext(profile)
        ->SetAPIRequestHelperForTesting(shared_url_loader_factory_);

    KeyringServiceFactory::GetServiceForContext(profile)->CreateWallet(
        "password_123", base::DoNothing());

    SetEthChainIdInterceptor(
        {GURL(kSomeEndpoint),
         GetKnownChain(profile->GetPrefs(), mojom::kFantomMainnetChainId,
                       mojom::CoinType::ETH)
             ->rpc_endpoints.front()},
        mojom::kFantomMainnetChainId);

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
    // Overriding native confirmation dialog so it always confirms.
    EXPECT_TRUE(
        EvalJs(settings_, "window.confirm = () => true").value.is_none());
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
    return wallet_->GetWebUI()->GetController()->GetAs<WalletPanelUI>();
  }

  void SetEthChainIdInterceptor(const std::vector<GURL>& network_urls,
                                const std::string& chain_id) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [=](const network::ResourceRequest& request) {
          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("eth_chainId") != std::string::npos) {
            const std::string response = base::StringPrintf(
                R"({"jsonrpc":"2.0","id":1,"result":"%s"})", chain_id.c_str());
            for (auto& url : network_urls)
              url_loader_factory_.AddResponse(url.spec(), response);
          }
        }));
  }

  void WaitForFantomNetworkUrl(const GURL& url) {
    auto* prefs = browser()->profile()->GetPrefs();

    if (GetNetworkURL(prefs, mojom::kFantomMainnetChainId,
                      mojom::CoinType::ETH) == url) {
      return;
    }

    base::RunLoop run_loop;
    PrefChangeRegistrar pref_change_registrar;
    pref_change_registrar.Init(prefs);
    pref_change_registrar.Add(
        kBraveWalletCustomNetworks,
        base::BindLambdaForTesting([&run_loop, &prefs, &url] {
          if (GetNetworkURL(prefs, mojom::kFantomMainnetChainId,
                            mojom::CoinType::ETH) == url) {
            run_loop.Quit();
          }
        }));
    run_loop.Run();
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

// This test is crashing on macos because renderer process DCHECKs trying
// to display scroll bar. Disabled for macos until this is fixed.
#if BUILDFLAG(IS_MAC)
#define MAYBE_HideNetworkInSettings DISABLED_HideNetworkInSettings
#else
#define MAYBE_HideNetworkInSettings HideNetworkInSettings
#endif
IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, MAYBE_HideNetworkInSettings) {
  ActivateWalletTab();
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Both Polygon and Fantom are listed.
  ASSERT_TRUE(WaitFor(wallet(), QuerySelectorJS(PolygonNetwork())));
  ASSERT_TRUE(
      EvalJs(wallet(), QuerySelectorJS(PolygonNetwork())).value.is_dict());
  ASSERT_TRUE(
      EvalJs(wallet(), QuerySelectorJS(FantomNetwork())).value.is_dict());

  // Wait and click on hide button for Fantom network in settings.
  CreateSettingsTab();
  ActivateSettingsTab();
  ASSERT_TRUE(WaitAndClickElement(
      settings(), SelectInNetworkList(FantomNetworkHideButton())));

  ActivateWalletTab();
  wallet()->GetController().Reload(content::ReloadType::NORMAL, true);
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Polygon is listed but Fantom is not.
  ASSERT_TRUE(WaitFor(wallet(), QuerySelectorJS(PolygonNetwork())));
  ASSERT_TRUE(
      EvalJs(wallet(), QuerySelectorJS(PolygonNetwork())).value.is_dict());
  ASSERT_TRUE(
      EvalJs(wallet(), QuerySelectorJS(FantomNetwork())).value.is_none());
}

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, CustomNetworkInSettings) {
  CreateSettingsTab();

  ActivateWalletTab();
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Fantom is listed in wallet.
  ASSERT_TRUE(WaitFor(wallet(), Select(FantomNetwork(), NetworkNameSpan()) +
                                    "?.innerText === 'Fantom Opera'"));

  // Go to wallet network settings and wait for Fantom network to appear.
  ActivateSettingsTab();
  ASSERT_TRUE(
      WaitFor(settings(), SelectInNetworkList(FantomNetworkChainName()) +
                              "?.innerText === 'Fantom Opera'"));

  // Double-click on Fantom network.
  ASSERT_TRUE(
      EvalJs(settings(), DoubleClickOn(SelectInNetworkList(FantomNetwork())))
          .ExtractBool());

  // Wait for edit network dialog with Fantom as chain name.
  ASSERT_TRUE(WaitFor(settings(), SelectInAddNetworkDialog("#chainName") +
                                      "?.value === 'Fantom Opera'"));

  // Change name to 'Custom Network'.
  ASSERT_EQ("Custom Network",
            EvalJs(settings(), SelectInAddNetworkDialog("#chainName") +
                                   ".value = 'Custom Network'")
                .ExtractString());

  // Click on submit button.
  ASSERT_TRUE(WaitAndClickElement(settings(),
                                  SelectInAddNetworkDialog(".action-button")));

  // Chain name for Fantom changes to 'Custom Network' in settings.
  ASSERT_TRUE(
      WaitFor(settings(), SelectInNetworkList(FantomNetworkChainName()) +
                              "?.innerText === 'Custom Network'"));

  // Chain name for Fantom changes to 'Custom Network' in wallet.
  ActivateWalletTab();
  ASSERT_TRUE(WaitFor(wallet(), Select(FantomNetwork(), NetworkNameSpan()) +
                                    "?.innerText === 'Custom Network'"));
}

IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest, SelectRpcEndpoint) {
  CreateSettingsTab();
  auto* prefs = browser()->profile()->GetPrefs();

  auto known_fantom_rpc =
      GetKnownChain(prefs, mojom::kFantomMainnetChainId, mojom::CoinType::ETH)
          ->rpc_endpoints.front();
  // Fantom rpc is from known info.
  WaitForFantomNetworkUrl(known_fantom_rpc);

  // Go to wallet network settings and wait for Fantom network to appear.
  ActivateSettingsTab();
  ASSERT_TRUE(
      WaitFor(settings(), SelectInNetworkList(FantomNetworkChainName()) +
                              "?.innerText === 'Fantom Opera'"));

  // Double-click on Fantom network.
  ASSERT_TRUE(
      EvalJs(settings(), DoubleClickOn(SelectInNetworkList(FantomNetwork())))
          .ExtractBool());

  // Wait for edit network dialog with Fantom as chain name.
  ASSERT_TRUE(WaitFor(settings(), SelectInAddNetworkDialog("#chainName") +
                                      "?.value === 'Fantom Opera'"));

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

  // Wait for custom endpoint for Fantom.
  WaitForFantomNetworkUrl(GURL(kSomeEndpoint));

  // Wait for custom endpoint listed for Fantom.
  ASSERT_TRUE(WaitFor(settings(),
                      SelectInNetworkList(FantomNetwork() + " .secondary") +
                          "?.innerText === '0xfa https://some.endpoint.com/'"));

  // Double-click on Fantom network.
  ASSERT_TRUE(
      EvalJs(settings(), DoubleClickOn(SelectInNetworkList(FantomNetwork())))
          .ExtractBool());
  // Wait for edit network dialog with Fantom as chain name.
  ASSERT_TRUE(WaitFor(settings(), SelectInAddNetworkDialog("#chainName") +
                                      "?.value === 'Fantom Opera'"));
  // Click on second item(known rpc) in rpc list.
  ASSERT_TRUE(WaitAndClickElement(
      settings(),
      SelectInAddNetworkDialog(
          "#rpcRadioGroup cr-radio-button:nth-of-type(2) cr-input")));
  // Sumbit changes.
  ASSERT_TRUE(WaitAndClickElement(
      settings(), SelectInAddNetworkDialog("cr-button.action-button")));

  // Wait for endpoint to become known one.
  WaitForFantomNetworkUrl(known_fantom_rpc);
}

}  // namespace brave_wallet
