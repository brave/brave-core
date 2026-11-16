// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_ui.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/webui/brave_settings_ui.h"
#include "brave/browser/ui/webui/brave_wallet/polkadot/polkadot_ui.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "url/origin.h"

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
  return "[data-testid='network-name']";
}

std::string NeonEVMNetworkHideButton() {
  return NeonEVMNetwork() + " .hide-network-button";
}

std::string NeonEVMNetworkChainName() {
  return NeonEVMNetwork() + " .chainName";
}

std::string NetworksButton() {
  return R"([data-testid='select-network-button'])";
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

// Probes the panel's FrameTree for a RenderFrameHost whose last committed URL
// matches the Polkadot bridge's.
content::RenderFrameHost* FindPolkadotBridgeFrame(content::WebContents* panel) {
  return content::FrameMatchingPredicateOrNullptr(
      panel->GetPrimaryPage(),
      base::BindRepeating(&content::FrameHasSourceUrl,
                          GURL(kUntrustedPolkadotURL)));
}

// A real Polkadot Asset Hub `SignerPayloadJSON`, carried verbatim from
// polkadot_api_impl_unittest.cc. Its `specVersion` names the runtime the
// Asset Hub metadata fixture holds, so the bridge frame can actually decode
// it, and the extrinsic it came from is on chain:
// https://assethub-polkadot.subscan.io/extrinsic/21550016-2
constexpr char kAssetHubPayloadJson[] =
    R"({
  "specVersion": "0x001e9808",
  "transactionVersion": "0x0000000f",
  "address": "16cM3vo8Ay8g9PxEaHFG2HZmZ8TYHf4gYAjBm3zAjezTCUMt",
  "assetId": null,
  "blockHash":
    "0xa4d849853e6d5f0df4e03b3fbc104d7937d8e85585fcbafa06290bfb4de081a9",
  "blockNumber": "0x0148d3ac",
  "era": "0xc400",
  "genesisHash":
    "0x68d56f15f85d3136970ec16946040bc1752654e906147f7e43e9d539d7c3de2f",
  "metadataHash": null,
  "method": )"
    "\"0x1f03051c000401000007093581e70e3001000003890dff6f1608140d01020800010100"
    "f8224098b97b27e917689228c41f23e2b06cc48666bc6eaec030169b441578400f01010100"
    "00040201090704000f00163982d4df06000d00040201090704000b00f0ab75a40d00010100"
    "2a975dad1c0fb19221a9288a0ae4b63ed29f3c625a73578117dde65482b8fc233102010907"
    "04010200040201090704000f00268d0c30d20601040100040100000700e40b5402080d0102"
    "0c0001030083cf32ace9e4c47165934f9a695fc23f96dee73d2ccb4f8d0d25b94af6838034"
    "8f8eea289c3183f0f0bcb2a1fa2ee9ee89434eba012ccb4f8d0d25b94af68380348f8eea28"
    "9c3183f0f0bcb2a1fa2ee9ee89434eba0103c83d82a9d2060300\","
    R"(
  "mode": 0,
  "nonce": "0x00000001",
  "signedExtensions": [
    "AuthorizeCall",
    "CheckNonZeroSender",
    "CheckSpecVersion",
    "CheckTxVersion",
    "CheckGenesis",
    "CheckMortality",
    "CheckNonce",
    "CheckWeight",
    "ChargeAssetTxPayment",
    "PrevalidateAttests",
    "CheckMetadataHash",
    "EthSetOrigin",
    "StorageWeightReclaim",
    "UnitTransactionExtension",
    "VerifyMultiSignature",
    "AsPgas",
    "AsDotnsGateway",
    "RestrictOrigins"
  ],
  "tip": "0x00000000000000000000000000000000",
  "version": 4,
  "withSignedTransaction": false
})";

// The bytes `kAssetHubPayloadJson` signs over. Also carried from
// polkadot_api_impl_unittest.cc, which checks the signature over these bytes
// against the one the on-chain extrinsic above bears. That makes this a real
// oracle rather than a snapshot: if the frame ordered the signed extensions
// differently than the runtime declares them, or encoded `method` with its
// compact length prefix, these bytes would differ.
constexpr std::string_view kAssetHubSignaturePayloadHex =
    "1f03051c000401000007093581e70e3001000003890dff6f1608140d01020800010100f8"
    "224098b97b27e917689228c41f23e2b06cc48666bc6eaec030169b441578400f01010100"
    "00040201090704000f00163982d4df06000d00040201090704000b00f0ab75a40d000101"
    "002a975dad1c0fb19221a9288a0ae4b63ed29f3c625a73578117dde65482b8fc23310201"
    "090704010200040201090704000f00268d0c30d20601040100040100000700e40b540208"
    "0d01020c0001030083cf32ace9e4c47165934f9a695fc23f96dee73d2ccb4f8d0d25b94a"
    "f68380348f8eea289c3183f0f0bcb2a1fa2ee9ee89434eba012ccb4f8d0d25b94af68380"
    "348f8eea289c3183f0f0bcb2a1fa2ee9ee89434eba0103c83d82a9d2060300c400040000"
    "0008981e000f00000068d56f15f85d3136970ec16946040bc1752654e906147f7e43e9d5"
    "39d7c3de2fa4d849853e6d5f0df4e03b3fbc104d7937d8e85585fcbafa06290bfb4de081"
    "a900";

// The `partial_fee` in the `RuntimeDispatchInfo` PolkadotMockRpc answers
// `TransactionPaymentApi_query_info` with.
constexpr brave_wallet::uint128_t kExpectedPartialFee{15937408476};

}  // namespace

namespace brave_wallet {

class WalletPanelUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    BraveSettingsUI::ShouldExposeElementsForTesting() = true;

    brave_wallet_service()->json_rpc_service()->SetAPIRequestHelperForTesting(
        url_loader_factory_.GetSafeWeakWrapper());

    brave_wallet_service()
        ->asset_ratio_service()
        ->EnableDummyPricesForTesting();

    // We need to prevent the wallet creation from being stuck waiting to
    // download a restrictions list that doesn't exist.
    WalletDataFilesInstaller::GetInstance().Reset();

    brave_wallet_service()->keyring_service()->CreateWallet("password_123",
                                                            base::DoNothing());

    SetEthChainIdInterceptor(
        {GURL(kSomeEndpoint), brave_wallet_service()
                                  ->network_manager()
                                  ->GetKnownChain(mojom::kNeonEVMMainnetChainId,
                                                  mojom::CoinType::ETH)
                                  ->rpc_endpoints.front()},
        mojom::kNeonEVMMainnetChainId);

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->ServeFilesFromDirectory(BraveWalletTestDataFolder());
    ASSERT_TRUE(https_server_->Start());

    CreateWalletTab();
    CreateDappTab();
    BindWalletPanelToDapp();
  }

  void TearDownOnMainThread() override {
    BraveSettingsUI::ShouldExposeElementsForTesting() = false;
    dapp_contents_ = nullptr;
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void CreateWalletTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(),
        GURL(std::string(kBraveUIWalletPanelURL) + "crypto/connections"),
        ui::mojom::WindowOpenDisposition::kNewForegroundTab,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    wallet_index_ = browser()->tab_strip_model()->active_index();
  }

  void CreateDappTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), https_server_->GetURL("/popup.html"),
        ui::mojom::WindowOpenDisposition::kNewForegroundTab,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
    dapp_contents_ = browser()->tab_strip_model()->GetActiveWebContents();
    ActivateWalletTab();
  }

  void CreateSettingsTab() {
    ui_test_utils::NavigateToURLWithDisposition(
        browser(), GURL(std::string(kWalletSettingsURL) + "/networks"),
        ui::mojom::WindowOpenDisposition::kNewForegroundTab,
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

  // The panel resolves its origin from the tab it is bound to. In production
  // the bubble or side panel binds it to the dapp tab the user is on.
  void BindWalletPanelToDapp() {
    GetWebUIController()->SetTabWebContents(dapp());
  }

  void ReloadWalletTab() {
    wallet()->GetController().Reload(content::ReloadType::NORMAL, true);
    EXPECT_TRUE(WaitForLoadStop(wallet()));
    BindWalletPanelToDapp();
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
    auto* prefs = browser()->GetProfile()->GetPrefs();

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
  content::WebContents* dapp() { return dapp_contents_; }

  BraveWalletService* brave_wallet_service() {
    return BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
  }

 private:
  int wallet_index_ = 0;
  int settings_index_ = 0;
  network::TestURLLoaderFactory url_loader_factory_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  raw_ptr<content::WebContents> dapp_contents_ = nullptr;
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
  ReloadWalletTab();
  // Wait and click on select network button.
  ASSERT_TRUE(WaitAndClickElement(wallet(), QuerySelectorJS(NetworksButton())));

  // Polygon is listed but Neon EVM is not.
  ASSERT_TRUE(WaitFor(wallet(), QuerySelectorJS(PolygonNetwork())));
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(PolygonNetwork())).is_dict());
  ASSERT_TRUE(EvalJs(wallet(), QuerySelectorJS(NeonEVMNetwork())).is_ok());
}

// Renderer crashes with libc++ alignment assertion on win32-x86
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_CustomNetworkInSettings DISABLED_CustomNetworkInSettings
#else
#define MAYBE_CustomNetworkInSettings CustomNetworkInSettings
#endif
IN_PROC_BROWSER_TEST_F(WalletPanelUIBrowserTest,
                       MAYBE_CustomNetworkInSettings) {
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
  ReloadWalletTab();
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

class PolkadotDisabledWalletPanelUIBrowserTest
    : public WalletPanelUIBrowserTest {
 public:
  void SetUp() override {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "false"}}}},
        {});

    WalletPanelUIBrowserTest::SetUp();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PolkadotDisabledWalletPanelUIBrowserTest,
                       NoPolkadotIframeBridge) {
  // Prove that when Polkadot dapp support is not enabled that the panel does
  // not have the <iframe/> appended to its DOM. Because of lag, probing the
  // FrameTree may yield false negatives so we use a normal JS selector call
  // which probes the DOM itself.

  ReloadWalletTab();
  EXPECT_EQ(false, EvalJs(wallet(),
                          "loadTimeData.getBoolean('"
                          "isPolkadotDAppSupportEnabled')")
                       .ExtractBool());
  EXPECT_EQ(0, EvalJs(wallet(),
                      absl::StrFormat("document.querySelectorAll(`iframe[src^="
                                      "\"%s\"]`).length",
                                      kUntrustedPolkadotURL))
                   .ExtractInt());
}

class PolkadotWalletPanelUIBrowserTest : public WalletPanelUIBrowserTest {
 public:
  void SetUp() override {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
        {});

    WalletPanelUIBrowserTest::SetUp();
  }

  void BindPanelHandler(mojo::Remote<mojom::PanelHandler>& panel_handler) {
    auto* webui = wallet()->GetPrimaryMainFrame()->GetWebUI();
    ASSERT_TRUE(webui);

    auto* wallet_panel_ui = webui->GetController()->GetAs<WalletPanelUI>();
    ASSERT_TRUE(wallet_panel_ui);

    wallet_panel_ui->BindPanelHandler(
        panel_handler.BindNewPipeAndPassReceiver());
  }

  bool HasPendingSignDetailsRequestsWaiting() {
    return !GetWebUIController()->polkadot_bridge_waiters_.empty();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PolkadotWalletPanelUIBrowserTest, IframeBridge) {
  // Prove that our wallet panel actually attempts a navigation to the Polkadot
  // bridge. Because the wallet panel is created as part of the test fixture
  // startup routines, we have to reload the panel after establishing the
  // TestNavigationObserver.

  content::TestNavigationObserver bridge_observer{GURL(kUntrustedPolkadotURL)};
  bridge_observer.WatchExistingWebContents();
  ReloadWalletTab();
  bridge_observer.Wait();
  EXPECT_TRUE(bridge_observer.last_navigation_succeeded());

  auto* bridge = FindPolkadotBridgeFrame(wallet());
  ASSERT_TRUE(bridge);
  EXPECT_EQ(url::Origin::Create(GURL(kUntrustedPolkadotURL)),
            bridge->GetLastCommittedOrigin());
  // Prove both RenderFrameHosts live in separate processes.
  EXPECT_TRUE(bridge->IsCrossProcessSubframe());

  // Prove that our iframe loaded the bundle and we were able to bind the
  // mojom::Receiver in the browser process.
  auto* bridge_web_ui = bridge->GetWebUI();
  ASSERT_TRUE(bridge_web_ui);
  auto* polkadot_ui =
      bridge_web_ui->GetController()->GetAs<polkadot::UntrustedPolkadotUI>();
  ASSERT_TRUE(polkadot_ui);
  EXPECT_TRUE(
      base::test::RunUntil([&]() { return polkadot_ui->IsBoundForTesting(); }));

  // Prove we didn't destroy the main wallet panel.
  EXPECT_EQ(true, EvalJs(wallet(), "!!document.getElementById('mountPoint')")
                      .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(PolkadotWalletPanelUIBrowserTest,
                       IframeBridgeCannotFetch) {
  // It's paramount that the iframe is air gapped and that the @polkadot/types
  // code can't exfiltrate any data via network calls.

  content::TestNavigationObserver bridge_observer{GURL(kUntrustedPolkadotURL)};
  bridge_observer.WatchExistingWebContents();
  ReloadWalletTab();
  bridge_observer.Wait();
  EXPECT_TRUE(bridge_observer.last_navigation_succeeded());

  auto* bridge = FindPolkadotBridgeFrame(wallet());
  ASSERT_TRUE(bridge);

  EXPECT_EQ("connect-src", EvalJs(bridge, R"(
       new Promise((resolve) => {
         document.addEventListener('securitypolicyviolation', (event) => {
           if (event.disposition === 'enforce' &&
               event.blockedURI.startsWith('https://rpc.polkadot.io')) {
             resolve(event.effectiveDirective);
           }
         });
         fetch('https://rpc.polkadot.io/').catch(() => {});
       })
   )")
                               .ExtractString());
}

// Drives `PanelHandler::GetPolkadotSignRequestDetails` end to end: a request
// queued the way a dapp's `signer.signPayload()` queues one, the real bridge
// frame decoding it with @polkadot/types, and a stub node pricing the
// extrinsic the frame hands back. Derives from
// PolkadotWalletPanelUIBrowserTest for its feature setup and for
// `BindPanelHandler`, which is what lets these tests hold a
// `mojom::PanelHandler` without standing up a wallet front-end.
class PolkadotSignRequestWalletPanelUIBrowserTest
    : public PolkadotWalletPanelUIBrowserTest {
 public:
  PolkadotWalletService* polkadot_wallet_service() {
    return brave_wallet_service()->GetPolkadotWalletService();
  }

  mojom::PolkadotBridge* polkadot_bridge() {
    return GetWebUIController()->GetPolkadotBridge();
  }

  // Stands PolkadotMockRpc in front of the service's RPC. The service came from
  // the keyed-service factory already wired to the profile's network stack, so
  // this seam is the only way to reach it.
  //
  // Call this from the test body rather than from setup: the mock CHECKs that
  // every request carries a chain URL it recognizes, and by the time a test
  // body runs the panel has finished booting, so none of the panel's own
  // traffic can trip it. Configure the returned mock, then `FinalizeSetup()`.
  PolkadotMockRpc* InstallMockRpc() {
    polkadot_wallet_service()->GetPolkadotRpc()->SetAPIRequestHelperForTesting(
        polkadot_url_loader_factory_.GetSafeWeakWrapper());

    polkadot_mock_rpc_ = std::make_unique<PolkadotMockRpc>(
        &polkadot_url_loader_factory_,
        brave_wallet_service()->network_manager());
    return polkadot_mock_rpc_.get();
  }

  // Queues a sign request and returns the id the service assigned it.
  int32_t AddSignRequest(const std::string& raw_payload_json) {
    auto request = mojom::SignPolkadotTransactionRequest::New();
    request->account_id = MakeIndexBasedAccountId(
        mojom::CoinType::DOT, mojom::KeyringId::kPolkadotMainnet,
        mojom::AccountKind::kDerived, 0);
    request->chain_id = mojom::ChainId::New(
        mojom::CoinType::DOT, std::string(mojom::kPolkadotMainnetAssetHub));
    request->raw_payload_json = raw_payload_json;
    request->metadata_bytes =
        ReadMetadataFixture("state_getMetadata_assethub_polkadot.json");
    request->chain_properties = mojom::PolkadotChainProperties::New(
        mojom::OptionalUint16::New(0), std::vector<uint32_t>{10},
        std::vector<std::string>{"DOT"});

    // Nothing in these tests approves or rejects, so the dapp's callback never
    // runs.
    polkadot_wallet_service()->AddSignPolkadotTransactionRequest(
        std::move(request), base::DoNothing());

    auto pending = polkadot_wallet_service()
                       ->GetPendingSignPolkadotTransactionRequestsSync();
    CHECK(!pending.empty());
    return pending.back()->id;
  }

  mojom::PolkadotSignRequestDetailsPtr GetSignRequestDetails(
      int32_t request_id) {
    mojo::Remote<mojom::PanelHandler> panel_handler;
    BindPanelHandler(panel_handler);
    CHECK(panel_handler.is_connected());

    base::test::TestFuture<mojom::PolkadotSignRequestDetailsPtr> future;
    panel_handler->GetPolkadotSignRequestDetails(request_id,
                                                 future.GetCallback());

    return future.Take();
  }

  // The frame is appended and navigated while the panel boots, so whether it
  // has bound by the time a test body runs is a race. Settling it here keeps a
  // binding failure from surfacing fifteen seconds later as a null result
  // indistinguishable from a decode failure.
  [[nodiscard]] bool WaitForPolkadotBridgeBound() {
    return base::test::RunUntil(
        [&]() { return GetWebUIController()->GetPolkadotBridge() != nullptr; });
  }

  void TearDownOnMainThread() override {
    // Explicitly clear our PolkadotMockRpc instance here. This is because we
    // instantiate it with a NetworkManager owned by the BraveWalletService
    // which is Profile-scoped. This means because we're in a browsertest, the
    // Profiles are destructed before the destructors of the fixtures are run,
    // so we'd have a UAF if we skipped this portion of the teardown.
    polkadot_url_loader_factory_.SetInterceptor({});
    polkadot_mock_rpc_.reset();
    PolkadotWalletPanelUIBrowserTest::TearDownOnMainThread();
  }

  void ExpectAssetHubDetails(
      const mojom::PolkadotSignRequestDetailsPtr& details) {
    ASSERT_TRUE(details);

    ASSERT_TRUE(details->fee);
    EXPECT_EQ(details->fee->partial_fee, Uint128ToMojom(kExpectedPartialFee));

    auto as_human =
        base::JSONReader::ReadDict(details->as_human, base::JSON_PARSE_RFC);
    ASSERT_TRUE(as_human) << details->as_human;
    const auto* section = as_human->FindString("section");
    const auto* call_method = as_human->FindString("method");
    ASSERT_TRUE(section);
    ASSERT_TRUE(call_method);
    EXPECT_EQ(*section, "polkadotXcm");
    EXPECT_EQ(*call_method, "execute");
    EXPECT_TRUE(as_human->Find("args"));
  }

  void ExpectStoredSignaturePayload(uint32_t request_id) {
    auto pending =
        polkadot_wallet_service()->GetPendingSignPolkadotTransactionRequest(
            request_id);
    ASSERT_TRUE(pending);
    ASSERT_TRUE(pending->signature_payload);
    EXPECT_EQ(base::HexEncodeLower(*pending->signature_payload),
              kAssetHubSignaturePayloadHex);
  }

 private:
  network::TestURLLoaderFactory polkadot_url_loader_factory_;
  std::unique_ptr<PolkadotMockRpc> polkadot_mock_rpc_;
};

IN_PROC_BROWSER_TEST_F(PolkadotSignRequestWalletPanelUIBrowserTest,
                       SignRequestDetails) {
  ASSERT_TRUE(WaitForPolkadotBridgeBound());
  InstallMockRpc()->FinalizeSetup();

  const int32_t request_id = AddSignRequest(kAssetHubPayloadJson);
  auto details = GetSignRequestDetails(request_id);
  ExpectAssetHubDetails(details);
  ExpectStoredSignaturePayload(request_id);
}

IN_PROC_BROWSER_TEST_F(PolkadotSignRequestWalletPanelUIBrowserTest,
                       SignRequestDetailsBeforeBridgeBinds) {
  // Prove that our WalletPanelHandler handles the case where a request comes in
  // before the bridge is established.

  InstallMockRpc()->FinalizeSetup();

  const int32_t request_id = AddSignRequest(kAssetHubPayloadJson);

  // We skip ReloadWalletTab() here because it explicitly calls
  // WaitForLoadStop(), which means that our Polkadot JS bundle is loaded and
  // evaluated by the time the function returns.
  content::TestNavigationManager bridge_nav(wallet(),
                                            GURL(kUntrustedPolkadotURL));
  wallet()->GetController().Reload(content::ReloadType::NORMAL, true);

  // Park the request for the iframe's URL here. This means the bundle can not
  // yet be loaded.
  ASSERT_TRUE(bridge_nav.WaitForRequestStart());
  BindWalletPanelToDapp();

  // Because we were likely previously bound, wait for the disconnect
  // explicitly.
  ASSERT_TRUE(base::test::RunUntil([&] { return !polkadot_bridge(); }));

  // We manually create the mojo::Remote to the wallet's panel handler and issue
  // the request.
  mojo::Remote<mojom::PanelHandler> panel_handler;
  BindPanelHandler(panel_handler);
  base::test::TestFuture<mojom::PolkadotSignRequestDetailsPtr> future;
  panel_handler->GetPolkadotSignRequestDetails(request_id,
                                               future.GetCallback());

  // Spin until we see the pending request emplaced, proving that our async task
  // still hasn't completed.
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return HasPendingSignDetailsRequestsWaiting(); }));
  ASSERT_FALSE(future.IsReady());

  // Finally load the iframe and its JS bundle.
  ASSERT_TRUE(bridge_nav.WaitForNavigationFinished());

  auto details = future.Take();
  ExpectAssetHubDetails(details);
  ExpectStoredSignaturePayload(request_id);
}

IN_PROC_BROWSER_TEST_F(PolkadotSignRequestWalletPanelUIBrowserTest,
                       SignRequestDetailsWithoutFee) {
  // If we fail to grab the estimated execution fee, still pass along the
  // .asHuman() output.

  ASSERT_TRUE(WaitForPolkadotBridgeBound());
  auto* mock_rpc = InstallMockRpc();
  mock_rpc->RejectPaymentInfoRequest();
  mock_rpc->FinalizeSetup();

  const int32_t request_id = AddSignRequest(kAssetHubPayloadJson);
  auto details = GetSignRequestDetails(request_id);
  ASSERT_TRUE(details);

  EXPECT_FALSE(details->fee);
  EXPECT_TRUE(
      base::JSONReader::ReadDict(details->as_human, base::JSON_PARSE_RFC))
      << details->as_human;

  ExpectStoredSignaturePayload(request_id);
}

IN_PROC_BROWSER_TEST_F(PolkadotSignRequestWalletPanelUIBrowserTest,
                       SignRequestDetailsUndecodablePayload) {
  // Prove that our decoding frame can handle invalid input.

  ASSERT_TRUE(WaitForPolkadotBridgeBound());
  InstallMockRpc()->FinalizeSetup();

  const int32_t request_id = AddSignRequest("not a signer payload");
  EXPECT_FALSE(GetSignRequestDetails(request_id));

  auto pending =
      polkadot_wallet_service()->GetPendingSignPolkadotTransactionRequest(
          request_id);
  ASSERT_TRUE(pending);
  EXPECT_FALSE(pending->signature_payload);
}

IN_PROC_BROWSER_TEST_F(PolkadotSignRequestWalletPanelUIBrowserTest,
                       SignRequestDetailsUnknownRequestId) {
  // Prove that querying a non-existent request returns nothing.

  ASSERT_TRUE(WaitForPolkadotBridgeBound());
  InstallMockRpc()->FinalizeSetup();

  const int32_t request_id = AddSignRequest(kAssetHubPayloadJson);
  EXPECT_FALSE(GetSignRequestDetails(request_id + 1));
}

}  // namespace brave_wallet
