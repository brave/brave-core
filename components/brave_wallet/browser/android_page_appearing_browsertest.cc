/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/files/scoped_temp_dir.h"
#include "base/strings/pattern.h"
#include "base/test/bind.h"
#include "base/test/scoped_run_loop_timeout.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/ui/webui/brave_wallet/android/android_wallet_page_ui.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/cosmetic_filters/browser/cosmetic_filters_resources.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/test/base/android/android_browser_test.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/browser/web_ui_controller_interface_binder.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_client.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"

namespace content {
class ConsoleObserver : public WebContentsObserver {
 public:
  explicit ConsoleObserver(content::WebContents* web_contents)
      : WebContentsObserver(web_contents) {}
  ~ConsoleObserver() override = default;

  const std::vector<WebContentsConsoleObserver::Message>& messages() const {
    return messages_;
  }

  void SetPattern(std::string pattern) {
    DCHECK(!pattern.empty()) << "An empty pattern will never match.";
    pattern_ = std::move(pattern);
  }

  bool Wait() { return waiter_helper_.Wait(); }

 private:
  void OnDidAddMessageToConsole(
      RenderFrameHost* source_frame,
      blink::mojom::ConsoleMessageLevel log_level,
      const std::u16string& message_contents,
      int32_t line_no,
      const std::u16string& source_id,
      const absl::optional<std::u16string>& untrusted_stack_trace) override {
    WebContentsConsoleObserver::Message message(
        {source_frame, log_level, message_contents, line_no, source_id});

    messages_.push_back(std::move(message));

    if (!pattern_.empty() &&
        !base::MatchPattern(base::UTF16ToUTF8(message_contents), pattern_)) {
      return;
    }

    waiter_helper_.OnEvent();
  }

  std::string pattern_;
  WaiterHelper waiter_helper_;
  std::vector<WebContentsConsoleObserver::Message> messages_;
};
}  // namespace content

namespace brave_wallet {
namespace {

constexpr char kTokenList[] = R"({
      "": {
        "name": "Ethereum",
        "symbol": "ETH",
        "logo": "333.svg",
        "erc20": true,
        "decimals": 18,
        "chainId": "0x1"
      },
      "0x4444444444444444444444444444444444444444": {
        "name": "44444444444",
        "logo": "4444.svg",
        "erc20": true,
        "symbol": "4444",
        "decimals": 18,
        "chainId": "0x89"
      }
     })";

constexpr char kGetBalanceResp[] = R"({
  "jsonrpc": "2.0",
  "id": 1,
  "result": "0x2b2d5d96e28a1aef98"
})";
constexpr char kGetTransactionCount[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x"
  })";
constexpr char kEthCallResp[] = R"({"jsonrpc":"2.0","id":1,"result":
"0x000000000000000000000000000000000000000000000000000000000000002000000
000000000000000000000000000000000000000000000000000000000010000000000000
000000000000000000000000000000000000000000000000020000000000000000000000
000000000000000000000000000000000000000000000000000000000000000000000000
000000000000000000000000000000000400000000000000000000000000000000000000
000000000000000000000000000"})";

constexpr char kGetRatios[] =
    R"({"payload":{"eth":{"usd":1883.79,"usd_timeframe_change":
    -0.025482150408}},"lastUpdated":"2023-06-23T10:07:19.372567186Z"})";
constexpr char kConsoleMarker[] = "WaitingCompleteMarkerMessage";
constexpr char kPrintConsoleMarkerScript[] = R"(setTimeout(() => {
  console.log("$1");
}, "10000");)";

constexpr char kPasswordBrave[] = "brave";

void BindCosmeticFiltersResourcesOnTaskRunner(
    mojo::PendingReceiver<cosmetic_filters::mojom::CosmeticFiltersResources>
        receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<cosmetic_filters::CosmeticFiltersResources>(
          g_brave_browser_process->ad_block_service()),
      std::move(receiver));
}

void BindCosmeticFiltersResources(
    content::RenderFrameHost* const frame_host,
    mojo::PendingReceiver<cosmetic_filters::mojom::CosmeticFiltersResources>
        receiver) {
  g_brave_browser_process->ad_block_service()->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&BindCosmeticFiltersResourcesOnTaskRunner,
                                std::move(receiver)));
}
}  // namespace

class TestWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  explicit TestWebUIControllerFactory(const std::string& source_name)
      : source_name_(source_name) {}

  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override {
    if (url.host_piece() == kWalletPageHost) {
      return std::make_unique<AndroidWalletPageUI>(web_ui, url);
    }

    return nullptr;
  }

  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override {
    if (url.SchemeIs(content::kChromeUIScheme) &&
        url.host_piece() == kWalletPageHost && url.path_piece() == "/swap") {
      return reinterpret_cast<content::WebUI::TypeID>(1);
    }

    return content::WebUI::kNoWebUI;
  }

  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override {
    return url.SchemeIs(content::kChromeUIScheme) || url == "about:blank";
  }

 private:
  std::string source_name_;
};

class AndroidPageAppearingBrowserTest : public PlatformBrowserTest {
 public:
  AndroidPageAppearingBrowserTest() {
    factory_ = std::make_unique<TestWebUIControllerFactory>(kWalletPageHost);
    content::WebUIControllerFactory::RegisterFactory(factory_.get());
    scoped_feature_list_.InitWithFeatures(
        {}, {features::kBraveWalletFilecoinFeature,
             features::kBraveWalletSolanaFeature});
  }

  void SetUpOnMainThread() override {
    InitWallet();
    SetEthChainIdInterceptor();
    PlatformBrowserTest::SetUpOnMainThread();
  }

  content::WebContents* GetActiveWebContents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  Profile* GetProfile() { return chrome_test_utils::GetProfile(this); }

  PrefService* GetPrefs() {
    return TabModelList::models()[0]->GetProfile()->GetPrefs();
  }

  base::FilePath get_temp_path() const { return temp_dir_.GetPath(); }
  int64_t file_size() const { return file_size_; }
  absl::optional<std::string> file_digest() const { return file_digest_; }

  const std::string GetConsoleMessages(
      const content::ConsoleObserver& console_observer) const {
    std::stringstream ss;
    for (auto const& msg : console_observer.messages()) {
      ss << msg.message << " [" << msg.log_level << "]"
         << "source_id:" << msg.source_id << " line_no:" << msg.line_no
         << std::endl;
    }
    return ss.str();
  }

 protected:
  class TestContentBrowserClient : public ChromeContentBrowserClient {
   public:
    TestContentBrowserClient() = default;
    TestContentBrowserClient(const TestContentBrowserClient&) = delete;
    TestContentBrowserClient& operator=(const TestContentBrowserClient&) =
        delete;
    ~TestContentBrowserClient() override = default;

    void RegisterBrowserInterfaceBindersForFrame(
        content::RenderFrameHost* render_frame_host,
        mojo::BinderMapWithContext<content::RenderFrameHost*>* map) override {
      ChromeContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
          render_frame_host, map);
      content::RegisterWebUIControllerInterfaceBinder<
          brave_wallet::mojom::PageHandlerFactory, AndroidWalletPageUI>(map);
      map->Add<cosmetic_filters::mojom::CosmeticFiltersResources>(
          base::BindRepeating(&BindCosmeticFiltersResources));
    }
  };
  TestContentBrowserClient test_content_browser_client_;

  void InitWallet() {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    content::SetBrowserClientForTesting(&test_content_browser_client_);

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);

    wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            GetProfile());
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(GetProfile());
    json_rpc_service_ =
        brave_wallet::JsonRpcServiceFactory::GetServiceForContext(GetProfile());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    asset_ratio_service_ =
        brave_wallet::AssetRatioServiceFactory::GetServiceForContext(
            GetProfile());
    asset_ratio_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);

    base::RunLoop run_loop;
    keyring_service_->RestoreWallet(
        kMnemonicDivideCruise, kPasswordBrave, false,
        base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();

    TokenListMap token_list_map;
    ASSERT_TRUE(
        ParseTokenList(kTokenList, &token_list_map, mojom::CoinType::ETH));
    BlockchainRegistry::GetInstance()->UpdateTokenList(
        std::move(token_list_map));

    brave_wallet::SetDefaultEthereumWallet(
        GetProfile()->GetPrefs(),
        brave_wallet::mojom::DefaultWallet::BraveWallet);
  }

  void SetEthChainIdInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          if (!request.request_body || !request.request_body->elements() ||
              request.request_body->elements()->empty()) {
            const GURL ratios_url(GetAssetRatioBaseURL());
            if (request.url.host() == ratios_url.host()) {
              url_loader_factory_.AddResponse(request.url.spec(), kGetRatios);
            }
            return;
          }

          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          url_loader_factory_.ClearResponses();
          if (request_string.find("eth_getBalance") != std::string::npos) {
            url_loader_factory_.AddResponse(request.url.spec(),
                                            kGetBalanceResp);
          } else if (request_string.find("eth_call") != std::string::npos) {
            url_loader_factory_.AddResponse(request.url.spec(), kEthCallResp);
          } else if (request_string.find("eth_getTransactionCount") !=
                     std::string::npos) {
            url_loader_factory_.AddResponse(request.url.spec(),
                                            kGetTransactionCount);
          }
        }));
  }

  void VerifyConsoleOutputNoErrors(
      const content::ConsoleObserver& console_observer,
      const blink::mojom::ConsoleMessageLevel max_accepted_log_level,
      const std::vector<std::string> ignore_patterns) {
    const std::vector<content::WebContentsConsoleObserver::Message>&
        console_messages = console_observer.messages();
    const int expected = static_cast<int>(max_accepted_log_level);
    for (const auto& msg : console_messages) {
      bool ignore_message{false};
      for (const auto& ignore_pattern : ignore_patterns) {
        if (!ignore_pattern.empty() &&
            (std::string::npos !=
             base::UTF16ToUTF8(msg.message).find(ignore_pattern))) {
          ignore_message = true;
          break;
        }
      }
      if (ignore_message) {
        LOG(INFO) << "Ignored message: " << msg.message;
        continue;
      }

      EXPECT_GE(expected, static_cast<int>(msg.log_level))
          << "Console must not contain errors" << std::endl
          << "Messages:" << std::endl
          << GetConsoleMessages(console_observer);
    }
  }

  void VerifyPage(const GURL url,
                  const std::vector<std::string> ignore_patterns) {
    content::NavigationController::LoadURLParams params(url);
    params.transition_type = ui::PageTransitionFromInt(
        ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR);

    auto* web_contents = GetActiveWebContents();

    content::ConsoleObserver console_observer(web_contents);
    console_observer.SetPattern(kConsoleMarker);
    web_contents->GetController().LoadURLWithParams(params);
    web_contents->GetOutermostWebContents()->Focus();
    EXPECT_TRUE(WaitForLoadStop(web_contents));
    EXPECT_TRUE(web_contents->GetLastCommittedURL() == url)
        << "Expected URL " << url << " but observed "
        << web_contents->GetLastCommittedURL();

    auto result = content::EvalJs(
        web_contents,
        base::ReplaceStringPlaceholders(kPrintConsoleMarkerScript,
                                        {kConsoleMarker}, nullptr),
        content::EXECUTE_SCRIPT_DEFAULT_OPTIONS, 1);
    EXPECT_TRUE(result.error.empty())
        << "Could not execute script: " << result.error;

    EXPECT_TRUE(console_observer.Wait());
    VerifyConsoleOutputNoErrors(console_observer,
                                blink::mojom::ConsoleMessageLevel::kWarning,
                                ignore_patterns);
  }

  base::ScopedTempDir temp_dir_;
  int64_t file_size_;
  absl::optional<std::string> file_digest_;

  std::unique_ptr<TestWebUIControllerFactory> factory_;
  base::test::ScopedFeatureList scoped_feature_list_;
  raw_ptr<brave_wallet::AssetRatioService> asset_ratio_service_;
  raw_ptr<brave_wallet::KeyringService> keyring_service_;
  raw_ptr<brave_wallet::JsonRpcService> json_rpc_service_;
  raw_ptr<brave_wallet::BraveWalletService> wallet_service_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  network::TestURLLoaderFactory url_loader_factory_;
};

IN_PROC_BROWSER_TEST_F(AndroidPageAppearingBrowserTest, TestSwapPageAppearing) {
  GURL url = GURL("chrome://wallet/swap");
  const std::vector<std::string> ignore_patterns = {
      "TypeError: Cannot read properties of undefined (reading 'forEach')",
      "Error calling jsonRpcService.getERC20TokenBalances",
      "Error querying balance:", "Error: An internal error has occurred",
      "Unable to fetch getTokenBalancesForChainId"};
  VerifyPage(url, ignore_patterns);
}

IN_PROC_BROWSER_TEST_F(AndroidPageAppearingBrowserTest, TestSendPageAppearing) {
  GURL url = GURL("chrome://wallet/send");
  const std::vector<std::string> ignore_patterns = {
      "TypeError: Cannot read properties of undefined (reading 'forEach')"};
  VerifyPage(url, ignore_patterns);
}

IN_PROC_BROWSER_TEST_F(AndroidPageAppearingBrowserTest,
                       TestDepositPageAppearing) {
  GURL url = GURL("chrome://wallet/deposit-funds");
  const std::vector<std::string> ignore_patterns = {
      "TypeError: Cannot read properties of undefined (reading 'forEach')"};
  VerifyPage(url, ignore_patterns);
}

IN_PROC_BROWSER_TEST_F(AndroidPageAppearingBrowserTest, TestBuyPageAppearing) {
  GURL url = GURL("chrome://wallet/fund-wallet");
  const std::vector<std::string> ignore_patterns = {
      "TypeError: Cannot read properties of undefined (reading 'forEach')"};
  VerifyPage(url, ignore_patterns);
}
}  // namespace brave_wallet
