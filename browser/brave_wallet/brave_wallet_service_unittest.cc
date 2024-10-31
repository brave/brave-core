/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/permissions/brave_permission_manager.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "build/build_config.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/country_codes/country_codes.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

using content::StoragePartition;
using testing::ElementsAre;
using testing::Eq;

namespace {

const char token_list_json[] = R"(
  {
   "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
    "name": "USD Coin",
    "logo": "usdc.png",
    "erc20": true,
    "erc721": false,
    "symbol": "USDC",
    "decimals": 6,
    "chainId": "0x1"
   },
   "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
     "name": "Crypto Kitties",
     "logo": "CryptoKitties-Kitty-13733.svg",
     "erc20": false,
     "erc721": true,
     "symbol": "CK",
     "decimals": 0,
     "chainId": "0x1"
   },
   "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
     "name": "Uniswap",
     "logo": "uni.svg",
     "erc20": true,
     "symbol": "UNI",
     "decimals": 18,
     "chainId": "0x1"
   }
  })";

const char sepolia_list_json[] = R"(
  {
   "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
    "name": "USD Coin",
    "logo": "usdc.png",
    "erc20": true,
    "erc721": false,
    "symbol": "USDC",
    "decimals": 6,
    "chainId": "0xaa36a7"
   },
   "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
     "name": "Crypto Kitties",
     "logo": "CryptoKitties-Kitty-13733.svg",
     "erc20": false,
     "erc721": true,
     "symbol": "CK",
     "decimals": 0,
     "chainId": "0xaa36a7"
   },
   "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
     "name": "Uniswap",
     "logo": "uni.svg",
     "erc20": true,
     "symbol": "UNI",
     "decimals": 18,
     "chainId": "0xaa36a7"
   }
  })";

const char solana_token_list_json[] = R"(
  {
    "So11111111111111111111111111111111111111112": {
      "name": "Wrapped SOL",
      "logo": "So11111111111111111111111111111111111111112.png",
      "erc20": false,
      "symbol": "SOL",
      "decimals": 9,
      "chainId": "0x65",
      "coingeckoId": "solana"
    },
    "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v": {
      "name": "USD Coin",
      "logo": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png",
      "erc20": false,
      "symbol": "USDC",
      "decimals": 6,
      "chainId": "0x65",
      "coingeckoId": "usd-coin"
    },
    "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ": {
      "name": "Tesla Inc.",
      "logo": "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
      "erc20": false,
      "symbol": "TSLA",
      "decimals": 8,
      "chainId": "0x65"
    }
  })";

const char interface_supported_response[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000001"
  })";

const char interface_not_supported_response[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000000"
  })";

constexpr char kBraveUrl[] = "https://brave.com";

class MockDataRemovalObserver : public StoragePartition::DataRemovalObserver {
 public:
  explicit MockDataRemovalObserver(StoragePartition* partition) {
    observation_.Observe(partition);
  }

  MOCK_METHOD4(OnStorageKeyDataCleared,
               void(uint32_t,
                    content::StoragePartition::StorageKeyMatcherFunction,
                    base::Time,
                    base::Time));

 private:
  base::ScopedObservation<StoragePartition,
                          StoragePartition::DataRemovalObserver>
      observation_{this};
};

}  // namespace

namespace brave_wallet {

void GetErrorCodeMessage(base::Value formed_response,
                         mojom::ProviderError* error,
                         std::string* error_message) {
  if (!formed_response.is_dict()) {
    *error = mojom::ProviderError::kSuccess;
    error_message->clear();
    return;
  }
  auto code = formed_response.GetDict().FindInt("code");
  if (code) {
    *error = static_cast<mojom::ProviderError>(*code);
  }
  const std::string* message = formed_response.GetDict().FindString("message");
  if (message) {
    *error_message = *message;
  }
}

class TestBraveWalletServiceObserver
    : public brave_wallet::BraveWalletServiceObserverBase {
 public:
  TestBraveWalletServiceObserver() = default;

  void OnDefaultEthereumWalletChanged(mojom::DefaultWallet wallet) override {
    default_ethereum_wallet_ = wallet;
    default_ethereum_wallet_changed_fired_ = true;
  }
  void OnDefaultSolanaWalletChanged(mojom::DefaultWallet wallet) override {
    default_solana_wallet_ = wallet;
    default_solana_wallet_changed_fired_ = true;
  }
  void OnDefaultBaseCurrencyChanged(const std::string& currency) override {
    currency_ = currency;
    default_base_currency_changed_fired_ = true;
  }
  void OnDefaultBaseCryptocurrencyChanged(
      const std::string& cryptocurrency) override {
    cryptocurrency_ = cryptocurrency;
    default_base_cryptocurrency_changed_fired_ = true;
  }

  void OnNetworkListChanged() override { network_list_changed_fired_ = true; }

  MOCK_METHOD1(OnDiscoverAssetsCompleted,
               void(std::vector<mojom::BlockchainTokenPtr>));

  mojom::DefaultWallet GetDefaultEthereumWallet() {
    return default_ethereum_wallet_;
  }
  mojom::DefaultWallet GetDefaultSolanaWallet() {
    return default_solana_wallet_;
  }
  bool DefaultEthereumWalletChangedFired() {
    return default_ethereum_wallet_changed_fired_;
  }
  bool DefaultSolanaWalletChangedFired() {
    return default_solana_wallet_changed_fired_;
  }
  std::string GetDefaultBaseCurrency() { return currency_; }
  std::string GetDefaultBaseCryptocurrency() { return cryptocurrency_; }
  bool DefaultBaseCurrencyChangedFired() {
    return default_base_currency_changed_fired_;
  }
  bool DefaultBaseCryptocurrencyChangedFired() {
    return default_base_cryptocurrency_changed_fired_;
  }
  bool OnNetworkListChangedFired() { return network_list_changed_fired_; }

  mojo::PendingRemote<brave_wallet::mojom::BraveWalletServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() {
    default_ethereum_wallet_changed_fired_ = false;
    default_solana_wallet_changed_fired_ = false;
    default_base_currency_changed_fired_ = false;
    default_base_cryptocurrency_changed_fired_ = false;
    network_list_changed_fired_ = false;
  }

 private:
  mojom::DefaultWallet default_ethereum_wallet_ =
      mojom::DefaultWallet::BraveWalletPreferExtension;
  mojom::DefaultWallet default_solana_wallet_ =
      mojom::DefaultWallet::BraveWalletPreferExtension;
  bool default_ethereum_wallet_changed_fired_ = false;
  bool default_solana_wallet_changed_fired_ = false;
  bool default_base_currency_changed_fired_ = false;
  bool default_base_cryptocurrency_changed_fired_ = false;
  bool network_list_changed_fired_ = false;
  std::string currency_;
  std::string cryptocurrency_;
  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      observer_receiver_{this};
};

class BraveWalletServiceUnitTest : public testing::Test {
 public:
  BraveWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~BraveWalletServiceUnitTest() override = default;

 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        features::kBraveWalletBitcoinFeature);

#if BUILDFLAG(IS_ANDROID)
    task_environment_.AdvanceClock(base::Days(2));
#else
    base::Time future_mock_time;
    if (base::Time::FromString("3000-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }
#endif

    histogram_tester_ = std::make_unique<base::HistogramTester>();
    bitcoin_test_rpc_server_ = std::make_unique<BitcoinTestRpcServer>();

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    builder.AddTestingFactory(
        brave_wallet::BraveWalletServiceFactory::GetInstance(),
        base::BindRepeating(
            [](scoped_refptr<network::SharedURLLoaderFactory>
                   shared_url_loader_factory,
               TestingPrefServiceSimple* local_state,
               content::BrowserContext* context)
                -> std::unique_ptr<KeyedService> {
              auto* profile = Profile::FromBrowserContext(context);
              return std::make_unique<BraveWalletService>(
                  shared_url_loader_factory,
                  BraveWalletServiceDelegate::Create(profile),
                  profile->GetPrefs(), local_state);
            },
            shared_url_loader_factory_, local_state_->Get()));
    profile_ = builder.Build();
    service_ = brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
        profile_.get());
    ASSERT_TRUE(service_.get());
    network_manager_ = service_->network_manager();
    json_rpc_service_ = service_->json_rpc_service();
    keyring_service_ = service_->keyring_service();
    bitcoin_wallet_service_ = service_->GetBitcoinWalletService();
    bitcoin_wallet_service_->SetUrlLoaderFactoryForTesting(
        bitcoin_test_rpc_server_->GetURLLoaderFactory());
    tx_service_ = service_->tx_service();
    observer_ = std::make_unique<TestBraveWalletServiceObserver>();
    service_->AddObserver(observer_->GetReceiver());

    profile_->SetPermissionControllerDelegate(
        base::WrapUnique(static_cast<permissions::BravePermissionManager*>(
            PermissionManagerFactory::GetInstance()
                ->BuildServiceInstanceForBrowserContext(profile_.get())
                .release())));

    auto* registry = BlockchainRegistry::GetInstance();
    TokenListMap token_list_map;
    ASSERT_TRUE(
        ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
    ASSERT_TRUE(ParseTokenList(sepolia_list_json, &token_list_map,
                               mojom::CoinType::ETH));
    ASSERT_TRUE(ParseTokenList(solana_token_list_json, &token_list_map,
                               mojom::CoinType::SOL));
    registry->UpdateTokenList(std::move(token_list_map));

    token1_ = GetRegistry()->GetTokenByAddress(
        mojom::kMainnetChainId, mojom::CoinType::ETH,
        "0x6B175474E89094C44Da98b954EedeAC495271d0F");
    ASSERT_TRUE(token1_);
    ASSERT_EQ(token1_->symbol, "USDC");

    token2_ = GetRegistry()->GetTokenByAddress(
        mojom::kMainnetChainId, mojom::CoinType::ETH,
        "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
    ASSERT_TRUE(token2_);
    ASSERT_EQ(token2_->symbol, "UNI");

    erc721_token_ = GetRegistry()->GetTokenByAddress(
        mojom::kMainnetChainId, mojom::CoinType::ETH,
        "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
    ASSERT_TRUE(erc721_token_);
    ASSERT_EQ(erc721_token_->symbol, "CK");

    wrapped_sol_ = GetRegistry()->GetTokenByAddress(
        mojom::kSolanaMainnet, mojom::CoinType::SOL,
        "So11111111111111111111111111111111111111112");
    ASSERT_TRUE(wrapped_sol_);
    ASSERT_EQ(wrapped_sol_->symbol, "SOL");

    sol_usdc_ = GetRegistry()->GetTokenByAddress(
        mojom::kSolanaMainnet, mojom::CoinType::SOL,
        "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
    ASSERT_TRUE(sol_usdc_);
    ASSERT_EQ(sol_usdc_->symbol, "USDC");

    sol_tsla_ = GetRegistry()->GetTokenByAddress(
        mojom::kSolanaMainnet, mojom::CoinType::SOL,
        "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ");
    ASSERT_TRUE(sol_tsla_);
    ASSERT_EQ(sol_tsla_->symbol, "TSLA");

    eth_token_ = mojom::BlockchainToken::New();
    eth_token_->contract_address = "";
    eth_token_->name = "Ethereum";
    eth_token_->symbol = "ETH";
    eth_token_->is_compressed = false;
    eth_token_->is_erc20 = false;
    eth_token_->is_erc721 = false;
    eth_token_->is_erc1155 = false;
    eth_token_->is_nft = false;
    eth_token_->decimals = 18;
    eth_token_->visible = true;
    eth_token_->chain_id = "0x1";
    eth_token_->coin = mojom::CoinType::ETH;
    eth_token_->spl_token_program = mojom::SPLTokenProgram::kUnsupported;

    bat_token_ = mojom::BlockchainToken::New();
    bat_token_->contract_address = "0x0D8775F648430679A709E98d2b0Cb6250d2887EF";
    bat_token_->name = "Basic Attention Token";
    bat_token_->symbol = "BAT";
    eth_token_->is_compressed = false;
    bat_token_->is_erc20 = true;
    bat_token_->is_erc721 = false;
    bat_token_->is_erc1155 = false;
    bat_token_->decimals = 18;
    bat_token_->visible = true;
    bat_token_->logo = "bat.png";
    bat_token_->chain_id = "0x1";
    bat_token_->coin = mojom::CoinType::ETH;
    bat_token_->spl_token_program = mojom::SPLTokenProgram::kUnsupported;

    sol_token_ = mojom::BlockchainToken::New(
        "", "Solana", "sol.png", false, false, false, false,
        mojom::SPLTokenProgram::kUnsupported, false, false, "SOL", 9, true, "",
        "", mojom::kSolanaMainnet, mojom::CoinType::SOL, false);
    fil_token_ = mojom::BlockchainToken::New(
        "", "Filecoin", "fil.png", false, false, false, false,
        mojom::SPLTokenProgram::kUnsupported, false, false, "FIL", 18, true, "",
        "", mojom::kFilecoinMainnet, mojom::CoinType::FIL, false);
  }

  void TearDown() override {
    profile_->SetPermissionControllerDelegate(nullptr);
  }

  mojom::BlockchainTokenPtr GetToken1() { return token1_.Clone(); }
  mojom::BlockchainTokenPtr GetToken2() { return token2_.Clone(); }
  mojom::BlockchainTokenPtr GetErc721Token() { return erc721_token_.Clone(); }
  mojom::BlockchainTokenPtr GetEthToken() { return eth_token_.Clone(); }
  mojom::BlockchainTokenPtr GetBatToken() { return bat_token_.Clone(); }
  mojom::BlockchainTokenPtr GetSolToken() { return sol_token_.Clone(); }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return network_manager_->GetNetworkURL(chain_id, coin);
  }

  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  BlockchainRegistry* GetRegistry() {
    return BlockchainRegistry::GetInstance();
  }

  void SetupWallet() {
    keyring_service_->CreateWalletInternal(kMnemonicDivideCruise,
                                           kTestWalletPassword, false, false);
  }

  void SetInterceptors(std::map<GURL, std::string> responses) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, responses](const network::ResourceRequest& request) {
          // If the request url is in responses, add that response
          auto it = responses.find(request.url);
          if (it != responses.end()) {
            std::string response = it->second;
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  void SetGetEthNftStandardInterceptor(
      const GURL& expected_url,
      const std::map<std::string, std::string>& interface_id_to_response) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_url,
         interface_id_to_response](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          std::string_view request_string(request.request_body->elements()
                                              ->at(0)
                                              .As<network::DataElementBytes>()
                                              .AsStringPiece());
          // Check if any of the interface ids are in the request
          // if so, return the response for that interface id
          // if not, do nothing
          for (const auto& [interface_id, response] :
               interface_id_to_response) {
            bool request_is_checking_interface =
                request_string.find(interface_id.substr(2)) !=
                std::string::npos;
            if (request_is_checking_interface) {
              url_loader_factory_.ClearResponses();
              url_loader_factory_.AddResponse(expected_url.spec(), response);
              return;
            }
          }
        }));
  }

  void GetUserAssets(const std::string& chain_id,
                     mojom::CoinType coin_type,
                     std::vector<mojom::BlockchainTokenPtr>* out_tokens) {
    base::RunLoop run_loop;
    service_->GetUserAssets(
        chain_id, coin_type,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> tokens) {
              *out_tokens = std::move(tokens);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  std::vector<mojom::BlockchainTokenPtr> GetUserAssets(
      const std::string& chain_id,
      mojom::CoinType coin_type) {
    std::vector<mojom::BlockchainTokenPtr> result;
    GetUserAssets(chain_id, coin_type, &result);
    return result;
  }

  bool AddUserAsset(mojom::BlockchainTokenPtr token) {
    bool out_success = false;
    base::RunLoop run_loop;
    service_->AddUserAsset(std::move(token),
                           base::BindLambdaForTesting([&](bool success) {
                             out_success = success;
                             run_loop.Quit();
                           }));
    run_loop.Run();
    return out_success;
  }

  bool RemoveUserAsset(mojom::BlockchainTokenPtr token) {
    bool out_success = false;
    base::RunLoop run_loop;
    service_->RemoveUserAsset(std::move(token),
                              base::BindLambdaForTesting([&](bool success) {
                                out_success = success;
                                run_loop.Quit();
                              }));
    run_loop.Run();
    return out_success;
  }

  bool SetUserAssetVisible(mojom::BlockchainTokenPtr token, bool visible) {
    bool out_success = false;
    base::RunLoop run_loop;
    service_->SetUserAssetVisible(std::move(token), visible,
                                  base::BindLambdaForTesting([&](bool success) {
                                    out_success = success;
                                    run_loop.Quit();
                                  }));
    run_loop.Run();
    return out_success;
  }

  bool SetAssetSpamStatus(mojom::BlockchainTokenPtr token, bool is_spam) {
    bool out_success = false;
    base::RunLoop run_loop;
    service_->SetAssetSpamStatus(std::move(token), is_spam,
                                 base::BindLambdaForTesting([&](bool success) {
                                   out_success = success;
                                   run_loop.Quit();
                                 }));
    run_loop.Run();
    return out_success;
  }

  void SetDefaultEthereumWallet(mojom::DefaultWallet default_wallet) {
    auto old_default_wallet = observer_->GetDefaultEthereumWallet();
    EXPECT_FALSE(observer_->DefaultEthereumWalletChangedFired());
    service_->SetDefaultEthereumWallet(default_wallet);
    task_environment_.RunUntilIdle();
    if (old_default_wallet != default_wallet) {
      EXPECT_TRUE(observer_->DefaultEthereumWalletChangedFired());
    } else {
      EXPECT_FALSE(observer_->DefaultEthereumWalletChangedFired());
    }
    EXPECT_EQ(default_wallet, observer_->GetDefaultEthereumWallet());
    observer_->Reset();
  }

  void SetDefaultSolanaWallet(mojom::DefaultWallet default_wallet) {
    auto old_default_wallet = observer_->GetDefaultSolanaWallet();
    EXPECT_FALSE(observer_->DefaultSolanaWalletChangedFired());
    service_->SetDefaultSolanaWallet(default_wallet);
    task_environment_.RunUntilIdle();
    if (old_default_wallet != default_wallet) {
      EXPECT_TRUE(observer_->DefaultSolanaWalletChangedFired());
    } else {
      EXPECT_FALSE(observer_->DefaultSolanaWalletChangedFired());
    }
    EXPECT_EQ(default_wallet, observer_->GetDefaultSolanaWallet());
    observer_->Reset();
  }

  void SetDefaultBaseCurrency(const std::string& currency) {
    auto old_currency = observer_->GetDefaultBaseCurrency();
    EXPECT_FALSE(observer_->DefaultBaseCurrencyChangedFired());
    service_->SetDefaultBaseCurrency(currency);
    task_environment_.RunUntilIdle();
    if (old_currency != currency) {
      EXPECT_TRUE(observer_->DefaultBaseCurrencyChangedFired());
    } else {
      EXPECT_FALSE(observer_->DefaultBaseCurrencyChangedFired());
    }
    EXPECT_EQ(currency, observer_->GetDefaultBaseCurrency());
    observer_->Reset();
  }

  void SetDefaultBaseCryptocurrency(const std::string& cryptocurrency) {
    auto old_cryptocurrency = observer_->GetDefaultBaseCryptocurrency();
    EXPECT_FALSE(observer_->DefaultBaseCryptocurrencyChangedFired());
    service_->SetDefaultBaseCryptocurrency(cryptocurrency);
    task_environment_.RunUntilIdle();
    if (old_cryptocurrency != cryptocurrency) {
      EXPECT_TRUE(observer_->DefaultBaseCryptocurrencyChangedFired());
    } else {
      EXPECT_FALSE(observer_->DefaultBaseCryptocurrencyChangedFired());
    }
    EXPECT_EQ(cryptocurrency, observer_->GetDefaultBaseCryptocurrency());
    observer_->Reset();
  }

  mojom::DefaultWallet GetDefaultEthereumWallet() {
    base::RunLoop run_loop;
    mojom::DefaultWallet default_wallet;
    service_->GetDefaultEthereumWallet(
        base::BindLambdaForTesting([&](mojom::DefaultWallet v) {
          default_wallet = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return default_wallet;
  }

  mojom::DefaultWallet GetDefaultSolanaWallet() {
    base::RunLoop run_loop;
    mojom::DefaultWallet default_wallet;
    service_->GetDefaultSolanaWallet(
        base::BindLambdaForTesting([&](mojom::DefaultWallet v) {
          default_wallet = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return default_wallet;
  }

  std::string GetDefaultBaseCurrency() {
    base::RunLoop run_loop;
    std::string default_currency;
    service_->GetDefaultBaseCurrency(
        base::BindLambdaForTesting([&](const std::string& v) {
          default_currency = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return default_currency;
  }

  std::string GetDefaultBaseCryptocurrency() {
    base::RunLoop run_loop;
    std::string default_cryptocurrency;
    service_->GetDefaultBaseCryptocurrency(
        base::BindLambdaForTesting([&](const std::string& v) {
          default_cryptocurrency = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return default_cryptocurrency;
  }

  void SimulateOnGetImportInfo(const std::string& new_password,
                               bool result,
                               const ImportInfo& info,
                               ImportError error,
                               bool* success_out,
                               std::string* error_message_out) {
    // People import with a blank default keyring, so clear it out
    keyring_service_->Reset();
    base::RunLoop run_loop;
    service_->OnGetImportInfo(
        new_password,
        base::BindLambdaForTesting(
            [&](bool success, const std::optional<std::string>& error_message) {
              *success_out = success;
              if (error_message) {
                *error_message_out = *error_message;
              }
              run_loop.Quit();
            }),
        result, info, error);
    run_loop.Run();
  }

  std::vector<mojom::SignMessageRequestPtr> GetPendingSignMessageRequests()
      const {
    base::RunLoop run_loop;
    std::vector<mojom::SignMessageRequestPtr> requests_out;
    service_->GetPendingSignMessageRequests(base::BindLambdaForTesting(
        [&](std::vector<mojom::SignMessageRequestPtr> requests) {
          for (const auto& request : requests) {
            requests_out.push_back(request.Clone());
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    return requests_out;
  }

  void CheckPasswordAndMnemonic(const std::string& new_password,
                                const std::string& in_mnemonic,
                                bool* valid_password,
                                bool* valid_mnemonic) {
    ASSERT_NE(valid_password, nullptr);
    ASSERT_NE(valid_mnemonic, nullptr);

    *valid_password = keyring_service_->ValidatePasswordInternal(new_password);

    base::RunLoop run_loop;
    keyring_service_->GetWalletMnemonic(
        new_password, base::BindLambdaForTesting(
                          [&](const std::optional<std::string>& mnemonic) {
                            *valid_mnemonic = (mnemonic == in_mnemonic);
                            run_loop.Quit();
                          }));
    run_loop.Run();
  }

  std::vector<std::string> GetAddresses() {
    std::vector<std::string> result;
    for (auto& account_info : keyring_service_->GetAllAccountInfos()) {
      if (account_info->account_id->coin == mojom::CoinType::ETH) {
        result.push_back(account_info->address);
      }
    }
    return result;
  }

  void AddSuggestToken(mojom::BlockchainTokenPtr suggested_token,
                       mojom::BlockchainTokenPtr expected_token,
                       bool approve,
                       bool run_switch_network = false) {
    mojom::AddSuggestTokenRequestPtr request =
        mojom::AddSuggestTokenRequest::New(
            MakeOriginInfo(url::Origin::Create(GURL(kBraveUrl))),
            suggested_token.Clone());
    base::RunLoop run_loop;
    service_->AddSuggestTokenRequest(
        request.Clone(),
        base::BindLambdaForTesting(
            [&](base::Value id, base::Value formed_response, const bool reject,
                const std::string& first_allowed_account,
                const bool update_bind_js_properties) {
              bool user_approved = false;
              if (formed_response.type() == base::Value::Type::BOOLEAN) {
                user_approved = formed_response.GetBool();
              }
              mojom::ProviderError error;
              std::string error_message;
              GetErrorCodeMessage(std::move(formed_response), &error,
                                  &error_message);
              if (run_switch_network) {
                EXPECT_FALSE(user_approved);
                EXPECT_EQ(error, mojom::ProviderError::kUserRejectedRequest);
                EXPECT_FALSE(error_message.empty());
              } else {
                EXPECT_EQ(approve, user_approved);
                EXPECT_EQ(error, mojom::ProviderError::kSuccess);
                EXPECT_TRUE(error_message.empty());
              }
              run_loop.Quit();
            }),
        base::Value());

    auto requests = GetPendingAddSuggestTokenRequests();
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests[0]->token, expected_token);

    if (run_switch_network) {
      json_rpc_service_->SetNetwork(mojom::kSepoliaChainId,
                                    mojom::CoinType::ETH, std::nullopt);
    } else {
      service_->NotifyAddSuggestTokenRequestsProcessed(
          approve, {suggested_token->contract_address});
    }
    run_loop.Run();

    requests = GetPendingAddSuggestTokenRequests();
    EXPECT_TRUE(requests.empty());
  }

  std::vector<mojom::AddSuggestTokenRequestPtr>
  GetPendingAddSuggestTokenRequests() const {
    base::RunLoop run_loop;
    std::vector<mojom::AddSuggestTokenRequestPtr> requests_out;
    service_->GetPendingAddSuggestTokenRequests(base::BindLambdaForTesting(
        [&](std::vector<mojom::AddSuggestTokenRequestPtr> requests) {
          for (const auto& request : requests) {
            requests_out.push_back(request.Clone());
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    return requests_out;
  }

  void GetNftDiscoveryEnabled(bool expected_enabled) {
    base::RunLoop run_loop;
    service_->GetNftDiscoveryEnabled(
        base::BindLambdaForTesting([&](bool enabled) {
          EXPECT_EQ(enabled, expected_enabled);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void GetSimpleHashSpamNFTs(
      const std::string& account_address,
      const std::vector<std::string>& chain_ids,
      mojom::CoinType coin,
      std::optional<std::string> cursor,
      const std::vector<mojom::BlockchainTokenPtr>& expected_nfts,
      std::optional<std::string> expected_cursor) {
    base::RunLoop run_loop;
    service_->GetSimpleHashSpamNFTs(
        account_address, chain_ids, coin, cursor,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> nfts,
                const std::optional<std::string>& returned_cursor) {
              ASSERT_EQ(nfts.size(), expected_nfts.size());
              EXPECT_EQ(returned_cursor, expected_cursor);
              EXPECT_EQ(nfts, expected_nfts);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  AccountUtils GetAccountUtils() { return AccountUtils(keyring_service_); }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  raw_ptr<BraveWalletService> service_ = nullptr;
  raw_ptr<NetworkManager> network_manager_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<TxService> tx_service_;
  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  raw_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  std::unique_ptr<TestBraveWalletServiceObserver> observer_;
  base::test::ScopedFeatureList scoped_feature_list_;

  mojom::BlockchainTokenPtr token1_;
  mojom::BlockchainTokenPtr token2_;
  mojom::BlockchainTokenPtr erc721_token_;
  mojom::BlockchainTokenPtr eth_token_;
  mojom::BlockchainTokenPtr bat_token_;
  mojom::BlockchainTokenPtr sol_token_;
  mojom::BlockchainTokenPtr wrapped_sol_;
  mojom::BlockchainTokenPtr sol_usdc_;
  mojom::BlockchainTokenPtr sol_tsla_;
  mojom::BlockchainTokenPtr fil_token_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BraveWalletServiceUnitTest, GetUserAssets) {
  std::vector<mojom::BlockchainTokenPtr> tokens;

  // Empty vector should be returned for invalid chain_id.
  GetUserAssets("", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens, std::vector<mojom::BlockchainTokenPtr>());

  GetUserAssets("0x123", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens, std::vector<mojom::BlockchainTokenPtr>());

  // Check mainnet default value.
  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());

  // Create ETH token with 0x5 chain_id.
  mojom::BlockchainTokenPtr eth_0x5_token = GetEthToken();
  eth_0x5_token->chain_id = "0xaa36a7";

  // ETH should be returned before any token is added.
  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], eth_0x5_token);

  // Prepare tokens to add.
  mojom::BlockchainTokenPtr token1 = GetToken1();
  mojom::BlockchainTokenPtr token2 = GetToken2();

  // Add tokens and test GetUserAsset.
  EXPECT_TRUE(AddUserAsset(token1.Clone()));

  // Adding token with lower case contract address should be converted to
  // checksum address.
  auto unchecked_token = token1.Clone();
  unchecked_token->chain_id = "0xaa36a7";
  unchecked_token->contract_address =
      base::ToLowerASCII(unchecked_token->contract_address);
  EXPECT_TRUE(AddUserAsset(std::move(unchecked_token)));

  auto token2_0xaa36a7 = token2.Clone();
  token2_0xaa36a7->chain_id = "0xaa36a7";
  EXPECT_TRUE(AddUserAsset(token2_0xaa36a7.Clone()));

  // Create Token1 with 0x1 chain_id.
  mojom::BlockchainTokenPtr token1_0x1 = token1.Clone();
  token1_0x1->chain_id = "0x1";

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(GetEthToken(), tokens[0]);
  EXPECT_EQ(GetBatToken(), tokens[1]);
  EXPECT_EQ(token1_0x1, tokens[2]);

  // Create Tokens with 0xaa36a7 chain_id.
  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";
  mojom::BlockchainTokenPtr token1_0xaa36a7 = token1.Clone();
  token1_0xaa36a7->chain_id = "0xaa36a7";

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
  EXPECT_EQ(token1_0xaa36a7, tokens[1]);
  EXPECT_EQ(token2_0xaa36a7, tokens[2]);

  // Remove token1 from "0x1" and token2 from "0xaa36a7" and test GetUserAssets.
  EXPECT_TRUE(RemoveUserAsset(token1_0x1.Clone()));

  EXPECT_TRUE(RemoveUserAsset(token2_0xaa36a7.Clone()));

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
  EXPECT_EQ(token1_0xaa36a7, tokens[1]);
}

TEST_F(BraveWalletServiceUnitTest, GetUserAssetsAlwaysHasNativeTokensForBtc) {
  GetPrefs()->SetList(kBraveWalletUserAssetsList, base::Value::List());

  auto btc_mainnet_token = GetBitcoinNativeToken(mojom::kBitcoinMainnet);
  auto btc_testnet_token = GetBitcoinNativeToken(mojom::kBitcoinTestnet);

  EXPECT_THAT(GetUserAssets(mojom::kBitcoinMainnet, mojom::CoinType::BTC),
              ElementsAre(Eq(std::ref(btc_mainnet_token))));
  EXPECT_THAT(GetUserAssets(mojom::kBitcoinTestnet, mojom::CoinType::BTC),
              ElementsAre(Eq(std::ref(btc_testnet_token))));

  btc_mainnet_token->visible = false;
  btc_testnet_token->visible = false;
  AddUserAsset(btc_mainnet_token.Clone());
  AddUserAsset(btc_testnet_token.Clone());

  EXPECT_THAT(GetUserAssets(mojom::kBitcoinMainnet, mojom::CoinType::BTC),
              ElementsAre(Eq(std::ref(btc_mainnet_token))));
  EXPECT_THAT(GetUserAssets(mojom::kBitcoinTestnet, mojom::CoinType::BTC),
              ElementsAre(Eq(std::ref(btc_testnet_token))));
}

TEST_F(BraveWalletServiceUnitTest, GetUserAssetsAlwaysHasNativeTokensForZec) {
  GetPrefs()->SetList(kBraveWalletUserAssetsList, base::Value::List());

  auto zec_mainnet_token = GetZcashNativeToken(mojom::kZCashMainnet);
  auto zec_testnet_token = GetZcashNativeToken(mojom::kZCashTestnet);

  EXPECT_THAT(GetUserAssets(mojom::kZCashMainnet, mojom::CoinType::ZEC),
              ElementsAre(Eq(std::ref(zec_mainnet_token))));
  EXPECT_THAT(GetUserAssets(mojom::kZCashTestnet, mojom::CoinType::ZEC),
              ElementsAre(Eq(std::ref(zec_testnet_token))));

  zec_mainnet_token->visible = false;
  zec_testnet_token->visible = false;
  AddUserAsset(zec_mainnet_token.Clone());
  AddUserAsset(zec_testnet_token.Clone());

  EXPECT_THAT(GetUserAssets(mojom::kZCashMainnet, mojom::CoinType::ZEC),
              ElementsAre(Eq(std::ref(zec_mainnet_token))));
  EXPECT_THAT(GetUserAssets(mojom::kZCashTestnet, mojom::CoinType::ZEC),
              ElementsAre(Eq(std::ref(zec_testnet_token))));
}

TEST_F(BraveWalletServiceUnitTest, DefaultAssets) {
  mojom::BlockchainTokenPtr eth_token = GetEthToken();
  mojom::BlockchainTokenPtr bat_token = GetBatToken();

  for (const auto& chain :
       network_manager_->GetAllKnownChains(mojom::CoinType::ETH)) {
    auto native_asset = mojom::BlockchainToken::New(
        "", chain->symbol_name, "", false, false, false, false,
        mojom::SPLTokenProgram::kUnsupported, false, false, chain->symbol,
        chain->decimals, true, "", "", chain->chain_id, mojom::CoinType::ETH,
        false);
    std::vector<mojom::BlockchainTokenPtr> tokens;
    GetUserAssets(chain->chain_id, mojom::CoinType::ETH, &tokens);
    if (chain->chain_id == mojom::kMainnetChainId) {
      EXPECT_EQ(tokens.size(), 2u);
      EXPECT_EQ(eth_token, tokens[0]);
      EXPECT_EQ(bat_token, tokens[1]);
    } else {
      SCOPED_TRACE(testing::PrintToString(chain->chain_id));
      EXPECT_EQ(tokens.size(), 1u);
      EXPECT_EQ(native_asset, tokens[0]);
    }
  }

  mojom::BlockchainTokenPtr sol_token = sol_token_->Clone();
  for (const auto& chain :
       network_manager_->GetAllKnownChains(mojom::CoinType::SOL)) {
    SCOPED_TRACE(testing::PrintToString(chain->chain_id));
    std::vector<mojom::BlockchainTokenPtr> tokens;
    sol_token->chain_id = chain->chain_id;
    GetUserAssets(chain->chain_id, mojom::CoinType::SOL, &tokens);
    EXPECT_EQ(tokens.size(), 1u);
    EXPECT_EQ(sol_token, tokens[0]);
  }

  mojom::BlockchainTokenPtr fil_token = fil_token_->Clone();
  for (const auto& chain :
       network_manager_->GetAllKnownChains(mojom::CoinType::FIL)) {
    SCOPED_TRACE(testing::PrintToString(chain->chain_id));
    std::vector<mojom::BlockchainTokenPtr> tokens;
    fil_token->chain_id = chain->chain_id;
    GetUserAssets(chain->chain_id, mojom::CoinType::FIL, &tokens);
    EXPECT_EQ(tokens.size(), 1u);
    EXPECT_EQ(fil_token, tokens[0]);
  }
}

TEST_F(BraveWalletServiceUnitTest, AddUserAsset) {
  std::vector<mojom::BlockchainTokenPtr> tokens;

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());

  mojom::BlockchainTokenPtr token = GetToken1();

  // Add token with empty contract address when there exists native asset
  // already should fail, in this case, it was eth.
  auto token_with_empty_contract_address = token.Clone();
  token_with_empty_contract_address->contract_address = "";
  token_with_empty_contract_address->chain_id = "0xaa36a7";
  EXPECT_FALSE(AddUserAsset(std::move(token_with_empty_contract_address)));

  // Invalid chain_id will fail.
  auto token_0x123 = token.Clone();
  token_0x123->chain_id = "0x123";
  EXPECT_FALSE(AddUserAsset(std::move(token_0x123)));

  // Add token.
  EXPECT_TRUE(AddUserAsset(token.Clone()));

  // Create Token1 with 0x1 chainId.
  mojom::BlockchainTokenPtr token1_0x1 = GetToken1();
  token1_0x1->chain_id = "0x1";

  // Check token is added as expected.
  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());
  EXPECT_EQ(tokens[2], token1_0x1);

  // Adding token with same address in the same chain will fail.
  EXPECT_FALSE(AddUserAsset(token.Clone()));

  // Adding token with same address in lower cases in the same chain will fail.
  auto token_with_unchecked_address = token.Clone();
  token_with_unchecked_address->contract_address =
      base::ToLowerASCII(token->contract_address);
  EXPECT_FALSE(AddUserAsset(token_with_unchecked_address.Clone()));

  // Create Tokens with 0xaa36a7 chain_id.
  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";
  mojom::BlockchainTokenPtr token1_0xaa36a7 = GetToken1();
  token1_0xaa36a7->chain_id = "0xaa36a7";

  // Adding token with same address in a different chain will succeed.
  // And the address will be converted to checksum address.
  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], eth_0xaa36a7_token);

  token_with_unchecked_address->chain_id = "0xaa36a7";
  EXPECT_TRUE(AddUserAsset(token_with_unchecked_address.Clone()));

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], eth_0xaa36a7_token);
  EXPECT_EQ(tokens[1], token1_0xaa36a7);
}

TEST_F(BraveWalletServiceUnitTest, AddUserAssetNfts) {
  std::map<std::string, std::string> responses;
  std::vector<mojom::BlockchainTokenPtr> tokens;
  GURL network = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);

  // is_erc721 is set to true based on supportsInterface call results.
  mojom::BlockchainTokenPtr erc721_token = mojom::BlockchainToken::New(
      "0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D", "BAYC", "bayc.png", false,
      false, false, false, mojom::SPLTokenProgram::kUnsupported, true, false,
      "BAYC", 0, true, "0x1", "", mojom::kMainnetChainId, mojom::CoinType::ETH,
      false);
  responses[kERC721InterfaceId] = interface_supported_response;
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_TRUE(AddUserAsset(erc721_token.Clone()));
  GetUserAssets(mojom::kMainnetChainId, mojom::CoinType::ETH, &tokens);
  ASSERT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[2]->contract_address, erc721_token->contract_address);
  EXPECT_EQ(tokens[2]->symbol, erc721_token->symbol);
  EXPECT_EQ(tokens[2]->name, erc721_token->name);
  EXPECT_EQ(tokens[2]->chain_id, erc721_token->chain_id);
  EXPECT_EQ(tokens[2]->decimals, erc721_token->decimals);
  EXPECT_EQ(tokens[2]->is_erc721, true);
  EXPECT_EQ(tokens[2]->is_erc1155, false);
  EXPECT_EQ(tokens[2]->is_erc20, false);

  // is_erc1155 is set to true based on supportsInterface call.
  mojom::BlockchainTokenPtr erc1155 = mojom::BlockchainToken::New(
      "0x28472a58A490c5e09A238847F66A68a47cC76f0f", "ADIDAS", "adidas.png",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, true,
      false, "ADIDAS", 0, true, "0x1", "", mojom::kMainnetChainId,
      mojom::CoinType::ETH, false);
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_TRUE(AddUserAsset(erc1155.Clone()));
  GetUserAssets(mojom::kMainnetChainId, mojom::CoinType::ETH, &tokens);
  ASSERT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[3]->contract_address, erc1155->contract_address);
  EXPECT_EQ(tokens[3]->symbol, erc1155->symbol);
  EXPECT_EQ(tokens[3]->name, erc1155->name);
  EXPECT_EQ(tokens[3]->chain_id, erc1155->chain_id);
  EXPECT_EQ(tokens[3]->decimals, erc1155->decimals);
  EXPECT_EQ(tokens[3]->is_erc721, false);
  EXPECT_EQ(tokens[3]->is_erc1155, true);
  EXPECT_EQ(tokens[3]->is_erc20, false);

  // A second ERC1155 token with same contract address, but different
  // token id is added.
  mojom::BlockchainTokenPtr erc1155_2 = mojom::BlockchainToken::New(
      "0x28472a58A490c5e09A238847F66A68a47cC76f0f", "ADIDAS", "adidas.png",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, true,
      false, "ADIDAS", 0, true, "0x2", "", mojom::kMainnetChainId,
      mojom::CoinType::ETH, false);
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_TRUE(AddUserAsset(erc1155_2.Clone()));
  GetUserAssets(mojom::kMainnetChainId, mojom::CoinType::ETH, &tokens);
  ASSERT_EQ(tokens.size(), 5u);
  EXPECT_EQ(tokens[4]->contract_address, erc1155_2->contract_address);
  EXPECT_EQ(tokens[4]->symbol, erc1155_2->symbol);
  EXPECT_EQ(tokens[4]->name, erc1155_2->name);
  EXPECT_EQ(tokens[4]->chain_id, erc1155_2->chain_id);
  EXPECT_EQ(tokens[4]->decimals, erc1155_2->decimals);
  EXPECT_EQ(tokens[4]->is_erc721, false);
  EXPECT_EQ(tokens[4]->is_erc1155, true);
  EXPECT_EQ(tokens[4]->is_erc20, false);

  // If invalid response is returned, AddUserAsset returns false.
  mojom::BlockchainTokenPtr erc1155_3 = mojom::BlockchainToken::New(
      "0x3333333333333333333333333333333333333333", "333333", "333333.png",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, true,
      false, "333333", 0, true, "0x1", "", mojom::kMainnetChainId,
      mojom::CoinType::ETH, false);
  responses[kERC721InterfaceId] = "invalid";
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_FALSE(AddUserAsset(erc1155_3.Clone()));

  // If neither erc721 nor erc1155 is supported, AddUserAsset returns false.
  mojom::BlockchainTokenPtr erc1155_4 = mojom::BlockchainToken::New(
      "0x4444444444444444444444444444444444444444", "444444", "444444.png",
      false, false, false, false, mojom::SPLTokenProgram::kUnsupported, true,
      false, "444444", 0, true, "0x1", "", mojom::kMainnetChainId,
      mojom::CoinType::ETH, false);
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_FALSE(AddUserAsset(erc1155_4.Clone()));
}

TEST_F(BraveWalletServiceUnitTest, RemoveUserAsset) {
  mojom::BlockchainTokenPtr token1 = GetToken1();
  mojom::BlockchainTokenPtr token2 = GetToken2();
  mojom::BlockchainTokenPtr token1_0x1 = GetToken1();
  token1_0x1->chain_id = "0x1";
  mojom::BlockchainTokenPtr token2_0x1 = GetToken2();
  token2_0x1->chain_id = "0x1";
  mojom::BlockchainTokenPtr token2_0xaa36a7 = GetToken2();
  token2_0xaa36a7->chain_id = "0xaa36a7";
  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";

  std::vector<mojom::BlockchainTokenPtr> tokens;

  // Add tokens
  EXPECT_TRUE(AddUserAsset(token1.Clone()));

  EXPECT_TRUE(AddUserAsset(token2.Clone()));

  EXPECT_TRUE(AddUserAsset(token2_0xaa36a7.Clone()));

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());
  EXPECT_EQ(tokens[2], token1_0x1);
  EXPECT_EQ(tokens[3], token2_0x1);

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], eth_0xaa36a7_token);
  EXPECT_EQ(tokens[1], token2_0xaa36a7);

  // Remove token with invalid contract_address returns false.
  auto invalid_eth_token = GetEthToken().Clone();
  invalid_eth_token->contract_address = "eth";
  EXPECT_FALSE(RemoveUserAsset(std::move(invalid_eth_token)));

  // Remove token with invalid network_id returns false.
  auto token1_0x123 = token1.Clone();
  token1_0x123->chain_id = "0x123";
  EXPECT_FALSE(RemoveUserAsset(std::move(token1_0x123)));

  // Returns false when we cannot find the list with network_id.
  auto token1_0x7 = token1.Clone();
  token1_0x7->chain_id = "0x7";
  EXPECT_FALSE(RemoveUserAsset(std::move(token1_0x7)));

  // Remove non-exist token returns false.
  auto token1_0xaa36a7 = token1.Clone();
  token1_0xaa36a7->chain_id = "0xaa36a7";
  EXPECT_FALSE(RemoveUserAsset(std::move(token1_0xaa36a7)));

  // Remove existing token.
  EXPECT_TRUE(RemoveUserAsset(token2.Clone()));

  // Lowercase address will be converted to checksum address when removing
  // token.
  auto BAT_lower_case_addr = GetBatToken();
  BAT_lower_case_addr->contract_address =
      base::ToLowerASCII(BAT_lower_case_addr->contract_address);
  EXPECT_TRUE(RemoveUserAsset(std::move(BAT_lower_case_addr)));

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], token1_0x1);
}

TEST_F(BraveWalletServiceUnitTest, SetUserAssetVisible) {
  mojom::BlockchainTokenPtr token1 = GetToken1();
  mojom::BlockchainTokenPtr token2 = GetToken2();
  mojom::BlockchainTokenPtr token1_0x1 = GetToken1();
  token1_0x1->chain_id = "0x1";
  mojom::BlockchainTokenPtr token2_0x1 = GetToken2();
  token2_0x1->chain_id = "0x1";
  mojom::BlockchainTokenPtr token2_0xaa36a7 = GetToken2();
  token2_0xaa36a7->chain_id = "0xaa36a7";
  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";

  std::vector<mojom::BlockchainTokenPtr> tokens;

  // Add tokens
  EXPECT_TRUE(AddUserAsset(token1.Clone()));

  EXPECT_TRUE(AddUserAsset(token2.Clone()));

  EXPECT_TRUE(AddUserAsset(token2_0xaa36a7.Clone()));

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());
  EXPECT_EQ(tokens[2], token1_0x1);
  EXPECT_EQ(tokens[3], token2_0x1);

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], eth_0xaa36a7_token);
  EXPECT_EQ(tokens[1], token2_0xaa36a7);

  // Invalid contract_address return false.
  auto invalid_eth = GetEthToken();
  invalid_eth->contract_address = "eth";
  EXPECT_FALSE(SetUserAssetVisible(std::move(invalid_eth), false));

  // Invalid chain_id return false.
  auto token1_0x123 = token1.Clone();
  token1_0x123->chain_id = "0x123";
  EXPECT_FALSE(SetUserAssetVisible(std::move(token1_0x123), false));

  // List for this network_id is not existed should return false.
  auto token1_0x5 = token1.Clone();
  token1_0x5->chain_id = "0xaa36a7";
  EXPECT_FALSE(SetUserAssetVisible(std::move(token1_0x5), false));

  auto token1_0xaa36a7 = token1.Clone();
  token1_0xaa36a7->chain_id = "0xaa36a7";
  // No entry with this contract address exists in the list.
  EXPECT_FALSE(SetUserAssetVisible(token1_0xaa36a7.Clone(), false));

  // Set visible to false for BAT & token1 in "0x1" and token2 in "0xaa36a7".
  EXPECT_TRUE(SetUserAssetVisible(token1.Clone(), false));

  // Lowercase address will be converted to checksum address directly.
  auto BAT_lower_case_addr = GetBatToken();
  BAT_lower_case_addr->contract_address =
      base::ToLowerASCII(BAT_lower_case_addr->contract_address);
  EXPECT_TRUE(SetUserAssetVisible(std::move(BAT_lower_case_addr), false));

  EXPECT_TRUE(SetUserAssetVisible(token2_0xaa36a7.Clone(), false));

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[0]->contract_address, GetEthToken()->contract_address);
  EXPECT_TRUE(tokens[0]->visible);
  EXPECT_EQ(tokens[1]->contract_address, GetBatToken()->contract_address);
  EXPECT_FALSE(tokens[1]->visible);
  EXPECT_EQ(tokens[2]->contract_address, token1->contract_address);
  EXPECT_FALSE(tokens[2]->visible);
  EXPECT_EQ(tokens[3]->contract_address, token2->contract_address);
  EXPECT_TRUE(tokens[3]->visible);

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0]->contract_address, GetEthToken()->contract_address);
  EXPECT_TRUE(tokens[0]->visible);
  EXPECT_EQ(tokens[1]->contract_address, token2->contract_address);
  EXPECT_FALSE(tokens[1]->visible);
}

TEST_F(BraveWalletServiceUnitTest, SetAssetSpamStatus) {
  mojom::BlockchainTokenPtr token1 = GetToken1();
  token1->chain_id = mojom::kMainnetChainId;
  std::vector<mojom::BlockchainTokenPtr> tokens;

  // Original list has two tokens
  GetUserAssets(token1->chain_id, mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);

  // Add token
  EXPECT_TRUE(AddUserAsset(token1.Clone()));

  GetUserAssets(token1->chain_id, mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_FALSE(tokens[2]->is_spam);  // New token is default not spam
  EXPECT_TRUE(tokens[2]->visible);   // New token should default to be visible

  // Flip spam
  EXPECT_TRUE(SetAssetSpamStatus(token1.Clone(), true));

  // Verify token has been set as spam and is not visible
  GetUserAssets(token1->chain_id, mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[2]->contract_address, token1->contract_address);
  EXPECT_TRUE(tokens[2]->is_spam);
  EXPECT_FALSE(tokens[2]->visible);  // Should not be visible since it's spam

  // Set asset as not spam
  EXPECT_TRUE(SetAssetSpamStatus(token1.Clone(), false));

  // Verify token has been set as not spam and is visible
  GetUserAssets(token1->chain_id, mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[2]->contract_address, token1->contract_address);
  EXPECT_FALSE(tokens[2]->is_spam);
  EXPECT_TRUE(tokens[2]->visible);  // Should be visible since it's not spam

  // Try to set spam status for non-existing asset
  mojom::BlockchainTokenPtr fakeToken = mojom::BlockchainToken::New();
  fakeToken->contract_address = "0xFakeAddress";
  fakeToken->chain_id = token1->chain_id;
  // Should fail because asset does not exist
  EXPECT_FALSE(SetAssetSpamStatus(fakeToken.Clone(), true));

  // Try to set spam status with invalid chain_id
  mojom::BlockchainTokenPtr tokenWithInvalidChain = token1.Clone();
  tokenWithInvalidChain->chain_id = "invalid_chain_id";
  // Should fail because of invalid chain_id
  EXPECT_FALSE(SetAssetSpamStatus(tokenWithInvalidChain.Clone(), true));

  // Set the spam_status of a token not in user assets list
  mojom::BlockchainTokenPtr token2 = GetToken1();
  token2->chain_id = mojom::kOptimismMainnetChainId;
  GetUserAssets(token2->chain_id, mojom::CoinType::ETH, &tokens);
  size_t original_optimism_user_asset_list = tokens.size();
  EXPECT_TRUE(SetAssetSpamStatus(token2.Clone(), true));
  GetUserAssets(token2->chain_id, mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), original_optimism_user_asset_list + 1u);
  EXPECT_EQ(tokens[1]->contract_address, token2->contract_address);
  EXPECT_EQ(tokens[1]->is_spam, true);
  EXPECT_EQ(tokens[1]->visible, false);
}

TEST_F(BraveWalletServiceUnitTest, GetAndSetDefaultEthereumWallet) {
  SetDefaultEthereumWallet(mojom::DefaultWallet::BraveWallet);
  EXPECT_EQ(GetDefaultEthereumWallet(), mojom::DefaultWallet::BraveWallet);

  SetDefaultEthereumWallet(mojom::DefaultWallet::CryptoWallets);
  EXPECT_EQ(GetDefaultEthereumWallet(), mojom::DefaultWallet::CryptoWallets);

  SetDefaultEthereumWallet(mojom::DefaultWallet::None);
  EXPECT_EQ(GetDefaultEthereumWallet(), mojom::DefaultWallet::None);

  SetDefaultEthereumWallet(mojom::DefaultWallet::BraveWalletPreferExtension);
  EXPECT_EQ(GetDefaultEthereumWallet(),
            mojom::DefaultWallet::BraveWalletPreferExtension);

  // Setting the same value twice is ok
  // SetDefaultEthereumWallet will check that the observer is not fired.
  SetDefaultEthereumWallet(mojom::DefaultWallet::BraveWalletPreferExtension);
  EXPECT_EQ(GetDefaultEthereumWallet(),
            mojom::DefaultWallet::BraveWalletPreferExtension);
}

TEST_F(BraveWalletServiceUnitTest, GetAndSetDefaultSolanaWallet) {
  SetDefaultSolanaWallet(mojom::DefaultWallet::BraveWallet);
  EXPECT_EQ(GetDefaultSolanaWallet(), mojom::DefaultWallet::BraveWallet);

  SetDefaultSolanaWallet(mojom::DefaultWallet::None);
  EXPECT_EQ(GetDefaultSolanaWallet(), mojom::DefaultWallet::None);

  SetDefaultSolanaWallet(mojom::DefaultWallet::BraveWalletPreferExtension);
  EXPECT_EQ(GetDefaultSolanaWallet(),
            mojom::DefaultWallet::BraveWalletPreferExtension);

  // Setting the same value twice is ok
  // SetDefaultSolanaWallet will check that the observer is not fired.
  SetDefaultSolanaWallet(mojom::DefaultWallet::BraveWalletPreferExtension);
  EXPECT_EQ(GetDefaultSolanaWallet(),
            mojom::DefaultWallet::BraveWalletPreferExtension);
}

TEST_F(BraveWalletServiceUnitTest, GetAndSetDefaultBaseCurrency) {
  SetDefaultBaseCurrency("CAD");
  EXPECT_EQ(GetDefaultBaseCurrency(), "CAD");

  // Setting the same value twice is ok
  // SetDefaultBaseCurrency will check that the observer is not fired.
  SetDefaultBaseCurrency("CAD");
  EXPECT_EQ(GetDefaultBaseCurrency(), "CAD");
}

TEST_F(BraveWalletServiceUnitTest, GetAndSetDefaultBaseCryptocurrency) {
  SetDefaultBaseCryptocurrency("ETH");
  EXPECT_EQ(GetDefaultBaseCryptocurrency(), "ETH");

  // Setting the same value twice is ok
  // SetDefaultBaseCryptocurrency will check that the observer is not fired.
  SetDefaultBaseCryptocurrency("ETH");
  EXPECT_EQ(GetDefaultBaseCryptocurrency(), "ETH");
}

TEST_F(BraveWalletServiceUnitTest, EthAddRemoveSetUserAssetVisible) {
  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";
  std::vector<mojom::BlockchainTokenPtr> tokens;

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);

  // Add ETH again will fail.
  EXPECT_FALSE(AddUserAsset(eth_0xaa36a7_token.Clone()));

  // Test setting visibility of ETH.
  EXPECT_TRUE(SetUserAssetVisible(eth_0xaa36a7_token.Clone(), false));

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_FALSE(tokens[0]->visible);

  // Test removing ETH from user asset list.
  EXPECT_TRUE(RemoveUserAsset(eth_0xaa36a7_token.Clone()));

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_TRUE(tokens.empty());

  // Add ETH with eth as the contract address will fail.
  auto invalid_eth = eth_0xaa36a7_token.Clone();
  invalid_eth->contract_address = "eth";
  EXPECT_FALSE(AddUserAsset(std::move(invalid_eth)));

  // Add ETH with empty contract address.
  EXPECT_TRUE(AddUserAsset(eth_0xaa36a7_token.Clone()));

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
}

TEST_F(BraveWalletServiceUnitTest, NetworkListChangedEvent) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x5566");

  network_manager_->AddCustomNetwork(chain);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(observer_->OnNetworkListChangedFired());

  // Remove network.
  observer_->Reset();
  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletCustomNetworks);
    base::Value::List* list = update->FindList(kEthereumPrefKey);
    list->EraseIf([&](const base::Value& v) {
      auto* chain_id_value = v.GetDict().FindString("chainId");
      if (!chain_id_value) {
        return false;
      }
      return *chain_id_value == "0x5566";
    });
  }
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(observer_->OnNetworkListChangedFired());
}

TEST_F(BraveWalletServiceUnitTest,
       CustomChainNativeAssetAddRemoveSetUserAssetVisible) {
  json_rpc_service_->SetSkipEthChainIdValidationForTesting(true);

  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x5566");

  json_rpc_service_->AddChain(chain.Clone(), base::DoNothing());

  auto native_asset = mojom::BlockchainToken::New(
      "", "symbol_name", "https://url1.com", false, false, false, false,
      mojom::SPLTokenProgram::kUnsupported, false, false, "symbol", 11, true,
      "", "", "0x5566", mojom::CoinType::ETH, false);

  std::vector<mojom::BlockchainTokenPtr> tokens;

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(native_asset.Clone(), tokens[0]);

  // Add native asset again will fail.
  EXPECT_FALSE(AddUserAsset(native_asset.Clone()));

  // Test setting visibility of ETH.
  EXPECT_TRUE(SetUserAssetVisible(native_asset.Clone(), false));

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_FALSE(tokens[0]->visible);

  // Test removing native asset from user asset list.
  EXPECT_TRUE(RemoveUserAsset(native_asset.Clone()));

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_TRUE(tokens.empty());

  // Add native asset again
  EXPECT_TRUE(AddUserAsset(native_asset.Clone()));

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(native_asset.Clone(), tokens[0]);
}

TEST_F(BraveWalletServiceUnitTest, AddCustomNetwork) {
  json_rpc_service_->SetSkipEthChainIdValidationForTesting(true);

  GetPrefs()->SetList(kBraveWalletUserAssetsList, base::Value::List());

  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  chain2.icon_urls.clear();

  json_rpc_service_->AddChain(chain1.Clone(), base::DoNothing());
  json_rpc_service_->AddChain(chain2.Clone(), base::DoNothing());

  // kBraveWalletCustomNetworks should be updated with new chains.
  ASSERT_EQ(2u,
            network_manager_->GetAllCustomChains(mojom::CoinType::ETH).size());
  EXPECT_EQ(chain1,
            *network_manager_->GetAllCustomChains(mojom::CoinType::ETH)[0]);
  EXPECT_EQ(chain2,
            *network_manager_->GetAllCustomChains(mojom::CoinType::ETH)[1]);

  // Asset list of new custom chains should have native asset in
  // kBraveWalletUserAssets.
  const auto& asset_list = GetPrefs()->GetList(kBraveWalletUserAssetsList);
  ASSERT_EQ(asset_list.size(), 2u);

  EXPECT_EQ(*asset_list[0].GetDict().FindInt("coin"),
            static_cast<int>(mojom::CoinType::ETH));
  EXPECT_EQ(*asset_list[0].GetDict().FindString("chain_id"), "chain_id");
  EXPECT_EQ(*asset_list[0].GetDict().FindString("address"), "");
  EXPECT_EQ(*asset_list[0].GetDict().FindString("name"), "symbol_name");
  EXPECT_EQ(*asset_list[0].GetDict().FindString("symbol"), "symbol");
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("is_erc20"), false);
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("is_erc721"), false);
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("is_erc1155"), false);
  EXPECT_EQ(*asset_list[0].GetDict().FindInt("decimals"), 11);
  EXPECT_EQ(*asset_list[0].GetDict().FindString("logo"), "https://url1.com");
  EXPECT_EQ(*asset_list[0].GetDict().FindBool("visible"), true);

  EXPECT_EQ(*asset_list[1].GetDict().FindInt("coin"),
            static_cast<int>(mojom::CoinType::ETH));
  EXPECT_EQ(*asset_list[1].GetDict().FindString("chain_id"), "chain_id2");
  EXPECT_EQ(*asset_list[1].GetDict().FindString("address"), "");
  EXPECT_EQ(*asset_list[1].GetDict().FindString("name"), "symbol_name2");
  EXPECT_EQ(*asset_list[1].GetDict().FindString("symbol"), "symbol2");
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("is_erc20"), false);
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("is_erc721"), false);
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("is_erc1155"), false);
  EXPECT_EQ(*asset_list[1].GetDict().FindInt("decimals"), 22);
  EXPECT_EQ(*asset_list[1].GetDict().FindString("logo"), "");
  EXPECT_EQ(*asset_list[1].GetDict().FindBool("visible"), true);

  {
    mojom::NetworkInfo chain_fil =
        GetTestNetworkInfo1(mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    json_rpc_service_->AddChain(chain_fil.Clone(), base::DoNothing());
    ASSERT_EQ(
        1u, network_manager_->GetAllCustomChains(mojom::CoinType::FIL).size());
    EXPECT_EQ(chain_fil,
              *network_manager_->GetAllCustomChains(mojom::CoinType::FIL)[0]);
  }

  {
    mojom::NetworkInfo chain_sol =
        GetTestNetworkInfo1(mojom::kSolanaMainnet, mojom::CoinType::SOL);
    json_rpc_service_->AddChain(chain_sol.Clone(), base::DoNothing());
    ASSERT_EQ(
        1u, network_manager_->GetAllCustomChains(mojom::CoinType::SOL).size());
    EXPECT_EQ(chain_sol,
              *network_manager_->GetAllCustomChains(mojom::CoinType::SOL)[0]);
  }

  {
    mojom::NetworkInfo chain_btc =
        GetTestNetworkInfo1(mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    json_rpc_service_->AddChain(chain_btc.Clone(), base::DoNothing());
    ASSERT_EQ(
        1u, network_manager_->GetAllCustomChains(mojom::CoinType::BTC).size());
    EXPECT_EQ(chain_btc,
              *network_manager_->GetAllCustomChains(mojom::CoinType::BTC)[0]);
  }

  {
    mojom::NetworkInfo chain_zec =
        GetTestNetworkInfo1(mojom::kZCashMainnet, mojom::CoinType::ZEC);
    json_rpc_service_->AddChain(chain_zec.Clone(), base::DoNothing());
    ASSERT_EQ(
        1u, network_manager_->GetAllCustomChains(mojom::CoinType::ZEC).size());
    EXPECT_EQ(chain_zec,
              *network_manager_->GetAllCustomChains(mojom::CoinType::ZEC)[0]);
  }

  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(BraveWalletServiceUnitTest, AddCustomNetworkTwice) {
  json_rpc_service_->SetSkipEthChainIdValidationForTesting(true);

  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();

  auto assets = GetAllUserAssets(GetPrefs());
  EXPECT_EQ(23u, assets.size());

  json_rpc_service_->AddChain(chain1.Clone(), base::DoNothing());

  assets = GetAllUserAssets(GetPrefs());
  EXPECT_EQ(24u, assets.size());

  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_TRUE(assets.back()->visible);

  SetUserAssetVisible(assets.back().Clone(), false);
  assets = GetAllUserAssets(GetPrefs());
  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_FALSE(assets.back()->visible);

  json_rpc_service_->RemoveChain(chain1.chain_id, chain1.coin,
                                 base::DoNothing());
  // TODO(apaymyshev): Maybe we should remove such assets.
  assets = GetAllUserAssets(GetPrefs());
  EXPECT_EQ(24u, assets.size());
  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_FALSE(assets.back()->visible);

  // Network added again. No duplicate assets.
  json_rpc_service_->AddChain(chain1.Clone(), base::DoNothing());
  assets = GetAllUserAssets(GetPrefs());
  EXPECT_EQ(24u, assets.size());
  EXPECT_EQ(assets.back()->name, chain1.symbol_name);
  EXPECT_TRUE(assets.back()->visible);
}

TEST_F(BraveWalletServiceUnitTest, ERC721TokenAddRemoveSetUserAssetVisible) {
  std::vector<mojom::BlockchainTokenPtr> tokens;

  auto erc721_token_with_empty_token_id = GetErc721Token();
  erc721_token_with_empty_token_id->chain_id = "0xaa36a7";
  auto erc721_token_1 = erc721_token_with_empty_token_id.Clone();
  erc721_token_1->token_id = "0x1";
  auto erc721_token_2 = erc721_token_with_empty_token_id.Clone();
  erc721_token_2->token_id = "0x2";
  auto erc721_token_1_ = erc721_token_with_empty_token_id.Clone();
  erc721_token_1->token_id = "0x1";

  // Add ERC721 token without tokenId will fail.
  auto network = GetNetwork(mojom::kSepoliaChainId, mojom::CoinType::ETH);
  std::map<std::string, std::string> responses;
  responses[kERC721InterfaceId] = interface_supported_response;
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_FALSE(AddUserAsset(std::move(erc721_token_with_empty_token_id)));

  // Add ERC721 token with token_id = 1 should success.
  EXPECT_TRUE(AddUserAsset(erc721_token_1.Clone()));

  // Add the same token_id should fail.
  EXPECT_FALSE(AddUserAsset(erc721_token_1.Clone()));

  // Add to another chain should success
  auto erc721_token_1_0x1 = erc721_token_1.Clone();
  erc721_token_1_0x1->chain_id = "0x1";
  network = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_TRUE(AddUserAsset(erc721_token_1_0x1.Clone()));

  // Add ERC721 token with token_id = 2 should success.
  network = GetNetwork(mojom::kSepoliaChainId, mojom::CoinType::ETH);
  SetGetEthNftStandardInterceptor(network, responses);
  EXPECT_TRUE(AddUserAsset(erc721_token_2.Clone()));

  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
  EXPECT_EQ(erc721_token_1, tokens[1]);
  EXPECT_EQ(erc721_token_2, tokens[2]);

  EXPECT_TRUE(SetUserAssetVisible(erc721_token_1.Clone(), false));

  EXPECT_TRUE(RemoveUserAsset(erc721_token_2.Clone()));

  auto erc721_token_1_visible_false = erc721_token_1.Clone();
  erc721_token_1_visible_false->visible = false;
  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
  EXPECT_EQ(erc721_token_1_visible_false, tokens[1]);
}

TEST_F(BraveWalletServiceUnitTest, SolanaTokenUserAssetsAPI) {
  std::vector<mojom::BlockchainTokenPtr> tokens;

  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 1u);
  EXPECT_EQ(sol_token_, tokens[0]);

  auto sol_token_devnet = sol_token_->Clone();
  sol_token_devnet->chain_id = mojom::kSolanaDevnet;
  GetUserAssets(mojom::kSolanaDevnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 1u);
  EXPECT_EQ(sol_token_devnet, tokens[0]);

  // Add usdc to mainnet and wrapped sol to devnet.
  EXPECT_TRUE(AddUserAsset(sol_usdc_->Clone()));
  auto wrapped_sol_devnet = wrapped_sol_->Clone();
  wrapped_sol_devnet->chain_id = mojom::kSolanaDevnet;
  EXPECT_TRUE(AddUserAsset(wrapped_sol_devnet->Clone()));

  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(sol_usdc_, tokens[1]);

  GetUserAssets(mojom::kSolanaDevnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(wrapped_sol_devnet, tokens[1]);

  // Set visible of wrapped sol to false on devnet
  EXPECT_TRUE(tokens[1]->visible);
  ASSERT_TRUE(SetUserAssetVisible(wrapped_sol_devnet->Clone(), false));
  GetUserAssets(mojom::kSolanaDevnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 2u);
  auto non_visible_wrapped_sol_devnet = wrapped_sol_devnet->Clone();
  non_visible_wrapped_sol_devnet->visible = false;
  EXPECT_EQ(non_visible_wrapped_sol_devnet, tokens[1]);

  // Remove usdc from mainnet and wrapped sol from devnet.
  EXPECT_TRUE(RemoveUserAsset(sol_usdc_->Clone()));
  EXPECT_TRUE(RemoveUserAsset(wrapped_sol_devnet->Clone()));
  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 1u);
  EXPECT_EQ(sol_token_, tokens[0]);
  GetUserAssets(mojom::kSolanaDevnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 1u);
  EXPECT_EQ(sol_token_devnet, tokens[0]);

  // Invalid chain id.
  GetUserAssets("0x100", mojom::CoinType::SOL, &tokens);
  EXPECT_TRUE(tokens.empty());
  auto sol_0x100 = sol_token_.Clone();
  sol_0x100->chain_id = "0x100";
  EXPECT_FALSE(AddUserAsset(sol_0x100->Clone()));
  EXPECT_FALSE(RemoveUserAsset(sol_0x100->Clone()));
  EXPECT_FALSE(SetUserAssetVisible(sol_0x100->Clone(), true));
}

TEST_F(BraveWalletServiceUnitTest, MigrateEip1559ForCustomNetworks) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  ASSERT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletEip1559ForCustomNetworksMigrated));
  GetPrefs()->ClearPref(kBraveWalletEip1559ForCustomNetworksMigrated);

  char legacy_custom_networks_pref[] =
      R"( {
            "ethereum": [ {
            "activeRpcEndpointIndex": 0,
            "blockExplorerUrls": [ "https://aurorascan.dev" ],
            "chainId": "0x4e454152",
            "chainName": "Aurora Mainnet Custom",
            "coin": 60,
            "iconUrls": [  ],
            "is_eip1559": false,
            "nativeCurrency": {
                "decimals": 18,
                "name": "Ether",
                "symbol": "ETH"
            },
            "rpcUrls": [ "https://mainnet-aurora.brave.com/" ]
          }, {
            "activeRpcEndpointIndex": 0,
            "blockExplorerUrls": [ "https://etherscan.io" ],
            "chainId": "0x1",
            "chainName": "Ethereum Mainnet Custom",
            "coin": 60,
            "iconUrls": [  ],
            "is_eip1559": true,
            "nativeCurrency": {
                "decimals": 18,
                "name": "Ethereum",
                "symbol": "ETH"
            },
            "rpcUrls": [ "https://mainnet-infura.brave.com/" ]
          }, {
            "activeRpcEndpointIndex": 0,
            "blockExplorerUrls": [ "https://lineascan.build" ],
            "chainId": "0xe708",
            "chainName": "Linea",
            "coin": 60,
            "iconUrls": [  ],
            "is_eip1559": true,
            "nativeCurrency": {
                "decimals": 18,
                "name": "Linea Ether",
                "symbol": "ETH"
            },
            "rpcUrls": [ "https://linea.blockpi.network/v1/rpc/public" ]
          } ],
          "solana": [ {
            "activeRpcEndpointIndex": 0,
            "blockExplorerUrls": [ "https://explorer.solana.com/?cluster=testnet" ],
            "chainId": "0x66",
            "chainName": "Solana Testnet Custom",
            "coin": 501,
            "iconUrls": [  ],
            "nativeCurrency": {
                "decimals": 9,
                "name": "Solana",
                "symbol": "SOL"
            },
            "rpcUrls": [ "https://api.testnet.solana.com/" ]
          } ]
        }
  )";

  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletEip1559ForCustomNetworksMigrated));
  GetPrefs()->SetBoolean(kSupportEip1559OnLocalhostChainDeprecated, false);
  GetPrefs()->SetDict(kBraveWalletCustomNetworks,
                      base::test::ParseJsonDict(legacy_custom_networks_pref));

  BraveWalletService::MigrateEip1559ForCustomNetworks(GetPrefs());
  for (auto&& [coin_key, value] :
       GetPrefs()->GetDict(kBraveWalletCustomNetworks)) {
    for (auto& custom_network : *value.GetIfList()) {
      EXPECT_FALSE(custom_network.GetDict().FindBool("is_eip1559"));
    }
  }

  EXPECT_FALSE(
      GetPrefs()->HasPrefPath(kSupportEip1559OnLocalhostChainDeprecated));

  EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletEip1559CustomChains),
            base::test::ParseJsonDict(R"( {
              "0x1": true,
              "0x4e454152": false,
              "0x539": false,
              "0xe708": true
            })"));

  EXPECT_FALSE(*network_manager_->IsEip1559Chain("0x4e454152"));
  EXPECT_TRUE(*network_manager_->IsEip1559Chain("0x1"));
  EXPECT_TRUE(*network_manager_->IsEip1559Chain("0xe708"));
  EXPECT_FALSE(*network_manager_->IsEip1559Chain(mojom::kLocalhostChainId));

  // solana does not get into this list.
  EXPECT_FALSE(network_manager_->IsEip1559Chain("0x66").has_value());

  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletEip1559ForCustomNetworksMigrated));
}

TEST_F(BraveWalletServiceUnitTest, MigrateDefaultHiddenNetworks) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  ASSERT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultHiddenNetworksVersion),
            1);
  GetPrefs()->SetInteger(kBraveWalletDefaultHiddenNetworksVersion, 0);

  BraveWalletService::MigrateHiddenNetworks(GetPrefs());
  {
    auto* list =
        GetPrefs()->GetDict(kBraveWalletHiddenNetworks).FindList("ethereum");
    ASSERT_NE(std::find_if(list->begin(), list->end(),
                           [](const auto& v) { return v == "0x4cb2f"; }),
              list->end());
  }
  ASSERT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultHiddenNetworksVersion),
            1);
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::ETH, "0x4cb2f");
  BraveWalletService::MigrateHiddenNetworks(GetPrefs());
  {
    auto* list =
        GetPrefs()->GetDict(kBraveWalletHiddenNetworks).FindList("ethereum");
    ASSERT_EQ(std::find_if(list->begin(), list->end(),
                           [](const auto& v) { return v == "0x4cb2f"; }),
              list->end());
  }
}

TEST_F(BraveWalletServiceUnitTest, MigrateDefaultHiddenNetworks_NoList) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  ASSERT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultHiddenNetworksVersion),
            1);
  GetPrefs()->SetInteger(kBraveWalletDefaultHiddenNetworksVersion, 0);

  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletHiddenNetworks);
    update.Get().Remove("ethereum");
  }
  BraveWalletService::MigrateHiddenNetworks(GetPrefs());
  {
    auto* list =
        GetPrefs()->GetDict(kBraveWalletHiddenNetworks).FindList("ethereum");
    EXPECT_NE(std::find_if(list->begin(), list->end(),
                           [](const auto& v) { return v == "0x4cb2f"; }),
              list->end());
  }
}

TEST_F(BraveWalletServiceUnitTest, MigrateFantomMainnetAsCustomNetwork) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  ASSERT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated));
  GetPrefs()->SetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated,
                         false);
  GetPrefs()->ClearPref(kBraveWalletCustomNetworks);
  GetPrefs()->ClearPref(kBraveWalletSelectedNetworksPerOrigin);

  // CASE 1: Fantom is the selected network of some origin
  ASSERT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated));

  auto selected_networks = base::JSONReader::Read(R"({
    "ethereum": {
      "https://app.uniswap.org": "0xfa"
    }
  })");
  GetPrefs()->Set(kBraveWalletSelectedNetworksPerOrigin, *selected_networks);

  EXPECT_FALSE(
      network_manager_->CustomChainExists("0xfa", mojom::CoinType::ETH));

  BraveWalletService::MigrateFantomMainnetAsCustomNetwork(GetPrefs());

  // OK: Fantom should be added to custom networks
  EXPECT_TRUE(
      network_manager_->CustomChainExists("0xfa", mojom::CoinType::ETH));

  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated));

  // CASE 2: Fantom is the default ETH network
  GetPrefs()->SetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated,
                         false);
  GetPrefs()->ClearPref(kBraveWalletCustomNetworks);
  GetPrefs()->ClearPref(kBraveWalletSelectedNetworksPerOrigin);

  auto default_networks = base::JSONReader::Read(R"({
    "ethereum": "0xfa"
  })");
  GetPrefs()->Set(kBraveWalletSelectedNetworks, *default_networks);

  EXPECT_FALSE(
      network_manager_->CustomChainExists("0xfa", mojom::CoinType::ETH));

  BraveWalletService::MigrateFantomMainnetAsCustomNetwork(GetPrefs());

  // OK: Fantom should be added to custom networks
  EXPECT_TRUE(
      network_manager_->CustomChainExists("0xfa", mojom::CoinType::ETH));

  // OK: default ETH network should be retained as Fantom
  EXPECT_EQ(
      *GetPrefs()->GetDict(kBraveWalletSelectedNetworks).FindString("ethereum"),
      "0xfa");

  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated));

  // CASE 3: Fantom neither default ETH network nor selected for any origin
  GetPrefs()->SetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated,
                         false);
  GetPrefs()->ClearPref(kBraveWalletCustomNetworks);
  GetPrefs()->ClearPref(kBraveWalletSelectedNetworksPerOrigin);

  default_networks = base::JSONReader::Read(R"({
    "ethereum": "0xa"
  })");
  GetPrefs()->Set(kBraveWalletSelectedNetworks, *default_networks);

  selected_networks = base::JSONReader::Read(R"({
    "ethereum": {
      "https://app.uniswap.org": "0x1"
    }
  })");
  GetPrefs()->Set(kBraveWalletSelectedNetworksPerOrigin, *selected_networks);

  EXPECT_FALSE(
      network_manager_->CustomChainExists("0xfa", mojom::CoinType::ETH));

  BraveWalletService::MigrateFantomMainnetAsCustomNetwork(GetPrefs());

  // KO: Fantom should NOT be added to custom networks
  EXPECT_FALSE(
      network_manager_->CustomChainExists("0xfa", mojom::CoinType::ETH));

  // KO: Default ETH network does not change
  EXPECT_EQ(
      *GetPrefs()->GetDict(kBraveWalletSelectedNetworks).FindString("ethereum"),
      "0xa");

  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated));

  // CASE 4: Fantom is already added to custom networks
  GetPrefs()->SetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated,
                         false);
  GetPrefs()->ClearPref(kBraveWalletCustomNetworks);
  GetPrefs()->ClearPref(kBraveWalletSelectedNetworksPerOrigin);

  // Add Fantom to custom networks
  mojom::NetworkInfo fantom = GetTestNetworkInfo1("0xfa");
  network_manager_->AddCustomNetwork(fantom);
  EXPECT_TRUE(
      network_manager_->CustomChainExists("0xfa", mojom::CoinType::ETH));

  BraveWalletService::MigrateFantomMainnetAsCustomNetwork(GetPrefs());

  // KO: Fantom should NOT be added to custom networks again
  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletCustomNetworks));
  auto* custom_networks =
      GetPrefs()->GetDict(kBraveWalletCustomNetworks).FindList("ethereum");
  ASSERT_TRUE(custom_networks);
  ASSERT_EQ(custom_networks->size(), 1u);
  EXPECT_EQ(*(*custom_networks)[0].GetDict().FindString("chainId"), "0xfa");

  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated));
}

TEST_F(BraveWalletServiceUnitTest, MigrateGoerliNetwork) {
  // Note: The testing profile has already performed the prefs migration by the
  // time this test runs, so undo its effects here for testing purposes
  ASSERT_TRUE(GetPrefs()->GetBoolean(kBraveWalletGoerliNetworkMigrated));
  GetPrefs()->SetBoolean(kBraveWalletGoerliNetworkMigrated, false);
  GetPrefs()->ClearPref(kBraveWalletSelectedNetworksPerOrigin);

  auto selected_networks = base::JSONReader::Read(R"({
    "ethereum": {
      "https://app.uniswap.org": "0xaa36a7"
    }
  })");
  GetPrefs()->Set(kBraveWalletSelectedNetworksPerOrigin, *selected_networks);

  auto default_networks = base::JSONReader::Read(R"({
    "ethereum": "0xaa36a7"
  })");
  GetPrefs()->Set(kBraveWalletSelectedNetworks, *default_networks);

  // CASE 1: Goerli is the selected network of some origin
  ASSERT_FALSE(GetPrefs()->GetBoolean(kBraveWalletGoerliNetworkMigrated));
  BraveWalletService::MigrateGoerliNetwork(GetPrefs());
  EXPECT_EQ(network_manager_->GetCurrentChainId(
                mojom::CoinType::ETH,
                url::Origin::Create(GURL("https://app.uniswap.org"))),
            mojom::kSepoliaChainId);
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletGoerliNetworkMigrated));

  // CASE 2: Goerli is the default ETH network
  GetPrefs()->SetBoolean(kBraveWalletGoerliNetworkMigrated, false);
  BraveWalletService::MigrateGoerliNetwork(GetPrefs());
  EXPECT_EQ(
      *GetPrefs()->GetDict(kBraveWalletSelectedNetworks).FindString("ethereum"),
      mojom::kSepoliaChainId);
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletGoerliNetworkMigrated));
}

TEST_F(BraveWalletServiceUnitTest, MigrateAssetsPrefToList) {
  ASSERT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsDeprecated));

  network_manager_->AddCustomNetwork(GetTestNetworkInfo1("0x123"));

  auto strip_coin_and_chain = [&](base::Value::Dict dict) -> base::Value::Dict {
    dict.Remove("chain_id");
    dict.Remove("coin");
    return dict;
  };

  auto eth_token = GetEthToken();
  auto bat_token = GetBatToken();
  auto custom_eth_token = GetEthToken();
  custom_eth_token->chain_id = "0x123";
  auto sol_token = GetSolToken();

  base::Value::Dict legacy_dict;
  base::Value::List legacy_eth_array;
  legacy_eth_array.Append(
      strip_coin_and_chain(BlockchainTokenToValue(eth_token)));
  legacy_eth_array.Append(
      strip_coin_and_chain(BlockchainTokenToValue(bat_token)));
  legacy_dict.SetByDottedPath("ethereum.mainnet", std::move(legacy_eth_array));

  base::Value::List legacy_custom_eth_array;
  legacy_custom_eth_array.Append(
      strip_coin_and_chain(BlockchainTokenToValue(custom_eth_token)));
  legacy_dict.SetByDottedPath("ethereum.0x123",
                              std::move(legacy_custom_eth_array));

  base::Value::List legacy_sol_array;
  legacy_sol_array.Append(
      strip_coin_and_chain(BlockchainTokenToValue(sol_token)));
  legacy_dict.SetByDottedPath("solana.mainnet", std::move(legacy_sol_array));

  GetPrefs()->SetDict(kBraveWalletUserAssetsDeprecated, std::move(legacy_dict));

  BraveWalletService::MigrateAssetsPrefToList(GetPrefs());

  ASSERT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsDeprecated));
  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsList));

  EXPECT_THAT(
      GetAllUserAssets(GetPrefs()),
      ElementsAre(Eq(std::ref(custom_eth_token)), Eq(std::ref(eth_token)),
                  Eq(std::ref(bat_token)), Eq(std::ref(sol_token))));
}

TEST_F(BraveWalletServiceUnitTest, OnGetImportInfo) {
  const char* new_password = "brave1234!";
  bool success;
  std::string error_message;
  SimulateOnGetImportInfo(new_password, false, ImportInfo(),
                          ImportError::kJsonError, &success, &error_message);
  EXPECT_FALSE(success);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_IMPORT_JSON_ERROR));

  SimulateOnGetImportInfo(new_password, false, ImportInfo(),
                          ImportError::kPasswordError, &success,
                          &error_message);
  EXPECT_FALSE(success);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_IMPORT_PASSWORD_ERROR));

  SimulateOnGetImportInfo(new_password, false, ImportInfo(),
                          ImportError::kInternalError, &success,
                          &error_message);
  EXPECT_FALSE(success);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_IMPORT_INTERNAL_ERROR));

  error_message.clear();
  const char* valid_mnemonic = kMnemonicDripCaution;
  SimulateOnGetImportInfo(new_password, true,
                          ImportInfo({valid_mnemonic, false, 3}),
                          ImportError::kNone, &success, &error_message);
  EXPECT_TRUE(success);
  EXPECT_TRUE(error_message.empty());
  {
    bool is_valid_password = false;
    bool is_valid_mnemonic = false;
    CheckPasswordAndMnemonic(new_password, valid_mnemonic, &is_valid_password,
                             &is_valid_mnemonic);
    EXPECT_TRUE(is_valid_password);
    EXPECT_TRUE(is_valid_mnemonic);

    const std::vector<std::string> expected_addresses(
        {"0x084DCb94038af1715963F149079cE011C4B22961",
         "0xE60A2209372AF1049C4848B1bF0136258c35f268",
         "0xb41c52De621B42A3a186ae1e608073A546195C9C"});
    EXPECT_EQ(expected_addresses, GetAddresses());
  }

  const char* valid_legacy_mnemonic =
      "cushion pitch impact album daring marine much annual budget social "
      "clarify "
      "balance rose almost area busy among bring hidden bind later capable "
      "pulp "
      "laundry";
  SimulateOnGetImportInfo(new_password, true,
                          ImportInfo({valid_legacy_mnemonic, true, 4}),
                          ImportError::kNone, &success, &error_message);
  EXPECT_TRUE(success);
  EXPECT_TRUE(error_message.empty());
  {
    bool is_valid_password = false;
    bool is_valid_mnemonic = false;
    CheckPasswordAndMnemonic(new_password, valid_legacy_mnemonic,
                             &is_valid_password, &is_valid_mnemonic);
    EXPECT_TRUE(is_valid_password);
    EXPECT_TRUE(is_valid_mnemonic);

    const std::vector<std::string> expected_addresses(
        {"0xea3C17c81E3baC3472d163b2c8b12ddDAa027874",
         "0xEc1BB5a4EC94dE9107222c103907CCC720fA3854",
         "0x8cb80Ef1d274ED215A4C08B31b77e5A813eD8Ea1",
         "0x3899D70A5D45368807E38Ef2c1EB5E4f07542e4f"});
    EXPECT_EQ(expected_addresses, GetAddresses());
  }

  const char* invalid_mnemonic = "not correct seed word";
  SimulateOnGetImportInfo(new_password, true,
                          ImportInfo({invalid_mnemonic, false, 2}),
                          ImportError::kNone, &success, &error_message);
  EXPECT_FALSE(success);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_MNEMONIC_ERROR));
}

TEST_F(BraveWalletServiceUnitTest, SignMessageHardware) {
  SetupWallet();

  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://brave.com")));
  auto expected_signature =
      mojom::EthereumSignatureBytes::New(std::vector<uint8_t>{1, 2, 3, 4, 5});
  // That should be hw account per test name.
  auto account_id = GetAccountUtils().EnsureEthAccount(0)->account_id.Clone();
  std::string domain = "{}";
  std::string message = "0xAB";
  auto request1 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 1, account_id.Clone(),
      mojom::SignDataUnion::NewEthStandardSignData(
          mojom::EthStandardSignData::New(message)),
      mojom::CoinType::ETH, mojom::kMainnetChainId);
  bool callback_is_called = false;
  service_->AddSignMessageRequest(
      std::move(request1),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::EthereumSignatureBytesPtr signature,
                                     const std::optional<std::string>& error) {
        ASSERT_TRUE(approved);
        EXPECT_EQ(signature, expected_signature);
        EXPECT_FALSE(error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(
      true, 1, expected_signature.Clone(), std::nullopt);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  service_->NotifySignMessageRequestProcessed(
      true, 1, expected_signature.Clone(), std::nullopt);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  callback_is_called = false;
  std::string expected_error = "error";
  auto request2 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 2, account_id.Clone(),
      mojom::SignDataUnion::NewEthStandardSignData(
          mojom::EthStandardSignData::New(message)),
      mojom::CoinType::ETH, mojom::kMainnetChainId);
  service_->AddSignMessageRequest(
      std::move(request2),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::EthereumSignatureBytesPtr signature,
                                     const std::optional<std::string>& error) {
        ASSERT_FALSE(approved);
        EXPECT_EQ(signature, expected_signature);
        ASSERT_TRUE(error);
        EXPECT_EQ(*error, expected_error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(
      false, 2, expected_signature.Clone(), expected_error);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
}

TEST_F(BraveWalletServiceUnitTest, SignMessage) {
  SetupWallet();

  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://brave.com")));
  std::string expected_signature = std::string("0xSiGnEd");
  auto account_id = GetAccountUtils().EnsureEthAccount(0)->account_id.Clone();
  std::string domain = "{}";
  std::string message = "0xAB";
  auto request1 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 1, account_id.Clone(),
      mojom::SignDataUnion::NewEthStandardSignData(
          mojom::EthStandardSignData::New(message)),
      mojom::CoinType::ETH, mojom::kMainnetChainId);
  bool callback_is_called = false;
  service_->AddSignMessageRequest(
      std::move(request1),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::EthereumSignatureBytesPtr signature,
                                     const std::optional<std::string>& error) {
        ASSERT_TRUE(approved);
        EXPECT_FALSE(signature);
        EXPECT_FALSE(error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(true, 1, nullptr, std::nullopt);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  service_->NotifySignMessageRequestProcessed(true, 1, nullptr, std::nullopt);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  callback_is_called = false;
  std::string expected_error = "error";
  auto request2 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 2, account_id.Clone(),
      mojom::SignDataUnion::NewEthStandardSignData(
          mojom::EthStandardSignData::New(message)),
      mojom::CoinType::ETH, mojom::kMainnetChainId);
  service_->AddSignMessageRequest(
      std::move(request2),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::EthereumSignatureBytesPtr signature,
                                     const std::optional<std::string>& error) {
        ASSERT_FALSE(approved);
        EXPECT_FALSE(signature);
        EXPECT_FALSE(error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(false, 2, nullptr, std::nullopt);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
}

TEST_F(BraveWalletServiceUnitTest, AddSuggestToken) {
  std::vector<std::string> chain_ids = {mojom::kMainnetChainId,
                                        mojom::kSepoliaChainId};

  const auto get_user_asset =
      [this](const std::string& chain_id,
             const std::string& contract_address) -> mojom::BlockchainTokenPtr {
    std::vector<mojom::BlockchainTokenPtr> tokens;
    GetUserAssets(chain_id, mojom::CoinType::ETH, &tokens);
    for (auto& token : tokens) {
      if (contract_address == token->contract_address) {
        return token.Clone();
      }
    }
    return nullptr;
  };

  for (const std::string& chain_id : chain_ids) {
    json_rpc_service_->SetNetwork(chain_id, mojom::CoinType::ETH, std::nullopt);
    mojom::BlockchainTokenPtr usdc_from_blockchain_registry =
        mojom::BlockchainToken::New(
            "0x6B175474E89094C44Da98b954EedeAC495271d0F", "USD Coin",
            "usdc.png", false, true, false, false,
            mojom::SPLTokenProgram::kUnsupported, false, false, "USDC", 6, true,
            "", "", chain_id, mojom::CoinType::ETH, false);
    ASSERT_EQ(usdc_from_blockchain_registry,
              GetRegistry()->GetTokenByAddress(
                  chain_id, mojom::CoinType::ETH,
                  "0x6B175474E89094C44Da98b954EedeAC495271d0F"));
    mojom::BlockchainTokenPtr usdc_from_user_assets =
        mojom::BlockchainToken::New(
            "0x6B175474E89094C44Da98b954EedeAC495271d0F", "USD Coin", "", false,
            true, false, false, mojom::SPLTokenProgram::kUnsupported, false,
            false, "USDC", 6, true, "", "", chain_id, mojom::CoinType::ETH,
            false);
    ASSERT_TRUE(service_->AddUserAssetInternal(usdc_from_user_assets.Clone()));

    mojom::BlockchainTokenPtr usdc_from_request = mojom::BlockchainToken::New(
        "0x6B175474E89094C44Da98b954EedeAC495271d0F", "USDC", "", false, true,
        false, false, mojom::SPLTokenProgram::kUnsupported, false, false,
        "USDC", 6, true, "", "", chain_id, mojom::CoinType::ETH, false);

    mojom::BlockchainTokenPtr custom_token = mojom::BlockchainToken::New(
        "0x6b175474e89094C44Da98b954eEdeAC495271d1e", "COLOR", "", false, true,
        false, false, mojom::SPLTokenProgram::kUnsupported, false, false,
        "COLOR", 18, true, "", "", chain_id, mojom::CoinType::ETH, false);

    // Case 1: Suggested token does not exist (no entry with the same contract
    // address) in BlockchainRegistry nor user assets.
    // Token should be in user asset list and is visible, and the data should be
    // the same as the one in the request.
    AddSuggestToken(custom_token.Clone(), custom_token.Clone(), true);
    auto token = get_user_asset(chain_id, custom_token->contract_address);
    EXPECT_EQ(token, custom_token);

    // Case 2: Suggested token exists (has an entry with the same contract
    // address) in BlockchainRegistry and user asset list and is visible.
    // Token should be in user asset list and is visible, and the data should be
    // the same as the one in the user asset list.
    AddSuggestToken(usdc_from_request.Clone(), usdc_from_user_assets.Clone(),
                    true);
    token = get_user_asset(chain_id, usdc_from_user_assets->contract_address);
    EXPECT_EQ(token, usdc_from_user_assets);

    // Case 3: Suggested token exists in BlockchainRegistry and user asset list
    // but is not visible. Token should be in user
    // asset list and is visible, and the data should be the same as the one in
    // the user asset list.
    ASSERT_TRUE(
        service_->SetUserAssetVisible(usdc_from_user_assets.Clone(), false));
    token = get_user_asset(chain_id, usdc_from_user_assets->contract_address);
    AddSuggestToken(usdc_from_request.Clone(), token.Clone(), true);
    token = get_user_asset(chain_id, usdc_from_user_assets->contract_address);
    EXPECT_EQ(token, usdc_from_user_assets);

    // Case 4: Suggested token exists in BlockchainRegistry but not in user
    // asset list. Token should be in user asset list and is visible, and the
    // data should be the same as the one in BlockchainRegistry.
    ASSERT_TRUE(service_->RemoveUserAsset(usdc_from_user_assets.Clone()));
    AddSuggestToken(usdc_from_request.Clone(),
                    usdc_from_blockchain_registry.Clone(), true);
    token = get_user_asset(chain_id,
                           usdc_from_blockchain_registry->contract_address);
    EXPECT_EQ(token, usdc_from_blockchain_registry);

    mojom::BlockchainTokenPtr usdt_from_user_assets =
        mojom::BlockchainToken::New(
            "0xdAC17F958D2ee523a2206206994597C13D831ec7", "Tether", "usdt.png",
            false, true, false, false, mojom::SPLTokenProgram::kUnsupported,
            false, false, "USDT", 6, true, "", "", chain_id,
            mojom::CoinType::ETH, false);
    ASSERT_TRUE(service_->AddUserAssetInternal(usdt_from_user_assets.Clone()));

    mojom::BlockchainTokenPtr usdt_from_request = mojom::BlockchainToken::New(
        "0xdAC17F958D2ee523a2206206994597C13D831ec7", "USDT", "", false, true,
        false, false, mojom::SPLTokenProgram::kUnsupported, false, false,
        "USDT", 18, true, "", "", chain_id, mojom::CoinType::ETH, false);

    // Case 5: Suggested token exists in user asset list and is visible, does
    // not exist in BlockchainRegistry. Token should be in user asset list and
    // is visible, and the data should be the same as the one in user asset
    // list.
    AddSuggestToken(usdt_from_request.Clone(), usdt_from_user_assets.Clone(),
                    true);
    token = get_user_asset(chain_id, usdt_from_user_assets->contract_address);
    EXPECT_EQ(token, usdt_from_user_assets);

    // Case 6: Suggested token exists in user asset list but is not visible,
    // does not exist in BlockchainRegistry. Token should be in user asset list
    // and is visible, and the data should be the same as the one in user asset
    // list.
    ASSERT_TRUE(
        service_->SetUserAssetVisible(usdt_from_user_assets.Clone(), false));
    token = get_user_asset(chain_id, usdt_from_user_assets->contract_address);
    AddSuggestToken(usdt_from_request.Clone(), token.Clone(), true);
    token = get_user_asset(chain_id, usdt_from_user_assets->contract_address);
    EXPECT_EQ(token, usdt_from_user_assets);

    // Call AddSuggestTokenRequest and switch network without
    // NotifyAddSuggestTokenRequestsProcessed being called should clear out the
    // pending request and AddSuggestTokenRequestCallback should be run with
    // kUserRejectedRequest error.
    mojom::BlockchainTokenPtr busd = mojom::BlockchainToken::New(
        "0x4Fabb145d64652a948d72533023f6E7A623C7C53", "Binance USD", "", false,
        true, false, false, mojom::SPLTokenProgram::kUnsupported, false, false,
        "BUSD", 18, true, "", "", chain_id, mojom::CoinType::ETH, false);
    AddSuggestToken(busd.Clone(), busd.Clone(), false,
                    true /* run_switch_network */);

    // Test reject request.
    mojom::BlockchainTokenPtr brb_from_request = mojom::BlockchainToken::New(
        "0x6B175474E89094C44Da98b954EedeAC495271d0A", "BRB", "", false, true,
        false, false, mojom::SPLTokenProgram::kUnsupported, false, false, "BRB",
        6, true, "", "", chain_id, mojom::CoinType::ETH, false);
    ASSERT_FALSE(service_->RemoveUserAsset(brb_from_request.Clone()));
    AddSuggestToken(brb_from_request.Clone(), brb_from_request.Clone(), false);
    token = get_user_asset(chain_id, brb_from_request->contract_address);
    EXPECT_FALSE(token);
  }
}

TEST_F(BraveWalletServiceUnitTest, Reset) {
  SetupWallet();

  SetDefaultBaseCurrency("CAD");
  SetDefaultBaseCryptocurrency("ETH");
  mojom::BlockchainTokenPtr token1 = GetToken1();
  EXPECT_TRUE(AddUserAsset(token1.Clone()));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsList));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kDefaultBaseCurrency));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kDefaultBaseCryptocurrency));
  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://brave.com")));
  auto account_id = GetAccountUtils().EnsureEthAccount(0)->account_id.Clone();
  std::string domain = "{}";
  std::string message = "0xAB";
  auto request1 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 1, account_id.Clone(),
      mojom::SignDataUnion::NewEthStandardSignData(
          mojom::EthStandardSignData::New(message)),
      mojom::CoinType::ETH, mojom::kMainnetChainId);
  service_->AddSignMessageRequest(
      std::move(request1),
      base::BindLambdaForTesting([](bool, mojom::EthereumSignatureBytesPtr,
                                    const std::optional<std::string>&) {}));
  mojom::BlockchainTokenPtr custom_token = mojom::BlockchainToken::New(
      "0x6b175474e89094C44Da98b954eEdeAC495271d1e", "COLOR", "", false, true,
      false, false, mojom::SPLTokenProgram::kUnsupported, false, false, "COLOR",
      18, true, "", "", "0x1", mojom::CoinType::ETH, false);
  AddSuggestToken(custom_token.Clone(), custom_token.Clone(), true);

#if !BUILDFLAG(IS_ANDROID)
  auto* partition = profile_->GetDefaultStoragePartition();
  MockDataRemovalObserver observer(partition);
  const auto page_storage_key_callback_valid =
      [&](StoragePartition::StorageKeyMatcherFunction callback) {
        return callback.Run(blink::StorageKey::CreateFirstParty(
            url::Origin::Create(GURL(kBraveUIWalletURL))));
      };
  const auto panel_storage_key_callback_valid =
      [&](StoragePartition::StorageKeyMatcherFunction callback) {
        return callback.Run(blink::StorageKey::CreateFirstParty(
            url::Origin::Create(GURL(kBraveUIWalletPanelURL))));
      };

  EXPECT_CALL(observer, OnStorageKeyDataCleared(
                            StoragePartition::REMOVE_DATA_MASK_ALL,
                            testing::Truly(page_storage_key_callback_valid),
                            base::Time(), base::Time::Max()));
  EXPECT_CALL(observer, OnStorageKeyDataCleared(
                            StoragePartition::REMOVE_DATA_MASK_ALL,
                            testing::Truly(panel_storage_key_callback_valid),
                            base::Time(), base::Time::Max()));
#endif

  const std::string eth_addr = "0x407637cC04893DA7FA4A7C0B58884F82d69eD448";
  const std::string sol_addr = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  auto origin = url::Origin::Create(GURL(kBraveUrl));
  auto* delegate = service_->GetDelegateForTesting();
  ASSERT_TRUE(delegate);

  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_ETHEREUM, profile_.get(), origin, eth_addr));
  ASSERT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
      blink::PermissionType::BRAVE_SOLANA, profile_.get(), origin, sol_addr));

  ASSERT_TRUE(delegate->HasPermission(mojom::CoinType::ETH, origin, eth_addr));
  ASSERT_TRUE(delegate->HasPermission(mojom::CoinType::SOL, origin, sol_addr));

  service_->Reset();

  EXPECT_FALSE(delegate->HasPermission(mojom::CoinType::ETH, origin, eth_addr));
  EXPECT_FALSE(delegate->HasPermission(mojom::CoinType::SOL, origin, sol_addr));

  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsList));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kDefaultBaseCurrency));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kDefaultBaseCryptocurrency));
  EXPECT_TRUE(service_->sign_message_requests_.empty());
  EXPECT_TRUE(service_->sign_message_callbacks_.empty());
  EXPECT_TRUE(service_->add_suggest_token_callbacks_.empty());
  EXPECT_TRUE(service_->add_suggest_token_requests_.empty());

#if !BUILDFLAG(IS_ANDROID)
  // Wait for async ClearDataForOrigin
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer));
#endif
}

TEST_F(BraveWalletServiceUnitTest, NewUserReturningMetric) {
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNewUserReturningHistogramName, 0, 1);
  GetLocalState()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());

  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNewUserReturningHistogramName, 2, 2);

  GetLocalState()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  task_environment_.RunUntilIdle();

  histogram_tester_->ExpectBucketCount(
      kBraveWalletNewUserReturningHistogramName, 3, 1);

  task_environment_.FastForwardBy(base::Days(6));
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNewUserReturningHistogramName, 1, 1);
}

TEST_F(BraveWalletServiceUnitTest, NewUserReturningMetricMigration) {
  GetLocalState()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());

  task_environment_.RunUntilIdle();
  GetLocalState()->SetTime(kBraveWalletP3AFirstUnlockTime, base::Time());
  GetLocalState()->SetTime(kBraveWalletP3ALastUnlockTime, base::Time());

  task_environment_.FastForwardBy(base::Hours(30));
  // Existing unlock timestamp should not trigger "new" value for new user
  // metric
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNewUserReturningHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Hours(30));
  histogram_tester_->ExpectBucketCount(
      kBraveWalletNewUserReturningHistogramName, 1, 2);
}

TEST_F(BraveWalletServiceUnitTest, LastUsageTimeMetric) {
  histogram_tester_->ExpectTotalCount(kBraveWalletLastUsageTimeHistogramName,
                                      0);

  GetLocalState()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  task_environment_.RunUntilIdle();

  histogram_tester_->ExpectUniqueSample(kBraveWalletLastUsageTimeHistogramName,
                                        1, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_->ExpectBucketCount(kBraveWalletLastUsageTimeHistogramName,
                                       2, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_->ExpectBucketCount(kBraveWalletLastUsageTimeHistogramName,
                                       3, 1);
  histogram_tester_->ExpectBucketCount(kBraveWalletLastUsageTimeHistogramName,
                                       1, 7);

  GetLocalState()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  task_environment_.RunUntilIdle();

  histogram_tester_->ExpectBucketCount(kBraveWalletLastUsageTimeHistogramName,
                                       1, 8);
}

TEST_F(BraveWalletServiceUnitTest, GetNftDiscoveryEnabled) {
  // Default should be off
  GetNftDiscoveryEnabled(false);

  // Setting to true should be reflected
  service_->SetNftDiscoveryEnabled(true);
  GetNftDiscoveryEnabled(true);

  // And back again
  service_->SetNftDiscoveryEnabled(false);
  GetNftDiscoveryEnabled(false);
}

TEST_F(BraveWalletServiceUnitTest, SetNftDiscoveryEnabled) {
  // Default should be off
  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletNftDiscoveryEnabled));

  // Setting NFT discovery enabled should update the pref and trigger asset
  // discovery
  EXPECT_CALL(*observer_, OnDiscoverAssetsCompleted(testing::_)).Times(1);
  service_->SetNftDiscoveryEnabled(true);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer_));
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletNftDiscoveryEnabled));

  // Unsetting NFT discovery enabled should update the pref and not trigger
  // asset discovery
  EXPECT_CALL(*observer_, OnDiscoverAssetsCompleted(testing::_)).Times(0);
  service_->SetNftDiscoveryEnabled(false);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(&observer_));
  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletNftDiscoveryEnabled));
}

TEST_F(BraveWalletServiceUnitTest, SetPrivateWindowsEnabled) {
  // Default should be off
  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletPrivateWindowsEnabled));

  // Setting private enabled should update the pref.
  service_->SetPrivateWindowsEnabled(true);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletPrivateWindowsEnabled));

  // Unsetting NFT discovery enabled should update the pref.
  service_->SetPrivateWindowsEnabled(false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletPrivateWindowsEnabled));
}

TEST_F(BraveWalletServiceUnitTest, RecordGeneralUsageMetrics) {
  histogram_tester_->ExpectTotalCount(kBraveWalletMonthlyHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kBraveWalletWeeklyHistogramName, 0);
  histogram_tester_->ExpectTotalCount(kBraveWalletDailyHistogramName, 0);

  GetLocalState()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  task_environment_.RunUntilIdle();

  histogram_tester_->ExpectUniqueSample(kBraveWalletMonthlyHistogramName, 1, 1);
  histogram_tester_->ExpectUniqueSample(kBraveWalletWeeklyHistogramName, 1, 1);
  histogram_tester_->ExpectUniqueSample(kBraveWalletDailyHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(7));

  histogram_tester_->ExpectUniqueSample(kBraveWalletMonthlyHistogramName, 1, 1);
  histogram_tester_->ExpectUniqueSample(kBraveWalletWeeklyHistogramName, 1, 1);
  histogram_tester_->ExpectUniqueSample(kBraveWalletDailyHistogramName, 1, 1);

  GetLocalState()->SetTime(kBraveWalletLastUnlockTime, base::Time::Now());
  task_environment_.RunUntilIdle();

  histogram_tester_->ExpectUniqueSample(kBraveWalletMonthlyHistogramName, 1, 2);
  histogram_tester_->ExpectUniqueSample(kBraveWalletWeeklyHistogramName, 1, 2);
  histogram_tester_->ExpectUniqueSample(kBraveWalletDailyHistogramName, 1, 2);
}

TEST_F(BraveWalletServiceUnitTest, GetBalanceScannerSupportedChains) {
  service_->GetBalanceScannerSupportedChains(
      base::BindLambdaForTesting([](const std::vector<std::string>& chains) {
        std::vector<std::string> expected_chains = {
            mojom::kMainnetChainId,         mojom::kBnbSmartChainMainnetChainId,
            mojom::kPolygonMainnetChainId,  mojom::kOptimismMainnetChainId,
            mojom::kArbitrumMainnetChainId, mojom::kAvalancheMainnetChainId,
        };
        ASSERT_EQ(chains.size(), expected_chains.size());
        EXPECT_EQ(chains, expected_chains);
      }));
}

TEST_F(BraveWalletServiceUnitTest, GetSimpleHashSpamNFTs) {
  std::vector<mojom::BlockchainTokenPtr> expected_nfts;
  std::string json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 100
        }
      },
      {
        "chain": "polygon",
        "contract_address": "0x2222222222222222222222222222222222222222",
        "token_id": "2",
        "contract": {
          "type": "ERC721",
          "symbol": "TWO"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";

  GURL url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum&wallet_addresses="
      "0x0000000000000000000000000000000000000000");
  std::map<GURL, std::string> responses;
  responses[url] = json;

  // First make the call with NFT discovery disabled,
  // no NFTs should be discovered
  GetSimpleHashSpamNFTs("0x0000000000000000000000000000000000000000",
                        {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                        std::nullopt, expected_nfts, std::nullopt);

  // Enable NFT discovery then try again, only the spam NFT should be discovered
  SetInterceptors(responses);
  service_->SetNftDiscoveryEnabled(true);
  auto nft1 = mojom::BlockchainToken::New();
  nft1->chain_id = mojom::kPolygonMainnetChainId;
  nft1->contract_address = "0x1111111111111111111111111111111111111111";
  nft1->token_id = "0x1";
  nft1->is_compressed = false;
  nft1->is_erc721 = true;
  nft1->is_erc1155 = false;
  nft1->is_erc20 = false;
  nft1->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
  nft1->is_nft = true;
  nft1->symbol = "ONE";
  nft1->coin = mojom::CoinType::ETH;
  expected_nfts.push_back(std::move(nft1));
  GetSimpleHashSpamNFTs("0x0000000000000000000000000000000000000000",
                        {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                        std::nullopt, expected_nfts, std::nullopt);
}

TEST_F(BraveWalletServiceUnitTest, EnsureSelectedAccountForChain) {
  const char* new_password = "brave1234!";
  bool success;
  std::string error_message;
  SimulateOnGetImportInfo(new_password, true,
                          ImportInfo({kMnemonicDripCaution, false, 1}),
                          ImportError::kNone, &success, &error_message);

  auto accounts = std::move(keyring_service_->GetAllAccountsSync()->accounts);
  auto eth_account_id = accounts[0]->account_id->Clone();
  auto sol_account_id = accounts[1]->account_id->Clone();
  EXPECT_EQ(eth_account_id->coin, mojom::CoinType::ETH);
  EXPECT_EQ(sol_account_id->coin, mojom::CoinType::SOL);

  // SOL account is selected by default.
  EXPECT_EQ(
      sol_account_id,
      keyring_service_->GetAllAccountsSync()->selected_account->account_id);

  // For solana network switch current account is ok.
  EXPECT_EQ(sol_account_id, service_->EnsureSelectedAccountForChainSync(
                                mojom::CoinType::SOL, mojom::kSolanaMainnet));
  EXPECT_EQ(
      sol_account_id,
      keyring_service_->GetAllAccountsSync()->selected_account->account_id);

  // For polygon network we switch to eth account.
  EXPECT_EQ(eth_account_id,
            service_->EnsureSelectedAccountForChainSync(
                mojom::CoinType::ETH, mojom::kPolygonMainnetChainId));
  EXPECT_EQ(
      eth_account_id,
      keyring_service_->GetAllAccountsSync()->selected_account->account_id);

  // For filecoin network there is no account, so eth account is still selected.
  EXPECT_EQ(mojom::AccountIdPtr(),
            service_->EnsureSelectedAccountForChainSync(
                mojom::CoinType::FIL, mojom::kFilecoinMainnet));
  EXPECT_EQ(
      eth_account_id,
      keyring_service_->GetAllAccountsSync()->selected_account->account_id);

  // Create fil account and it gets selected.
  auto fil_account_id =
      keyring_service_
          ->AddAccountSync(mojom::CoinType::FIL, mojom::KeyringId::kFilecoin,
                           "Fil 1")
          ->account_id.Clone();
  EXPECT_EQ(
      fil_account_id,
      keyring_service_->GetAllAccountsSync()->selected_account->account_id);

  // Switch to eth account again.
  EXPECT_EQ(eth_account_id,
            service_->EnsureSelectedAccountForChainSync(
                mojom::CoinType::ETH, mojom::kPolygonMainnetChainId));
  EXPECT_EQ(
      eth_account_id,
      keyring_service_->GetAllAccountsSync()->selected_account->account_id);

  // As there is filecoin account we can switch to it.
  EXPECT_EQ(fil_account_id, service_->EnsureSelectedAccountForChainSync(
                                mojom::CoinType::FIL, mojom::kFilecoinMainnet));
  EXPECT_EQ(
      fil_account_id,
      keyring_service_->GetAllAccountsSync()->selected_account->account_id);
}

TEST_F(BraveWalletServiceUnitTest, ConvertFEVMToFVMAddress) {
  {
    bool callback_is_called;

    service_->ConvertFEVMToFVMAddress(
        true, {"0xf563fc08AFD59e2260b7d2d4481215c745Fa7573", "", "123"},
        base::BindLambdaForTesting(
            [&](const base::flat_map<std::string, std::string>& result) {
              EXPECT_EQ(
                  result.find("0xf563fc08AFD59e2260b7d2d4481215c745Fa7573")
                      ->second,
                  "f410f6vr7ycfp2wpceyfx2lkeqeqvy5c7u5ltn7oripq");
              EXPECT_EQ(result.size(), 1u);
              callback_is_called = true;
            }));
    ASSERT_TRUE(callback_is_called);
  }

  {
    bool callback_is_called;

    service_->ConvertFEVMToFVMAddress(
        false, {"0xf563fc08AFD59e2260b7d2d4481215c745Fa7573", "", "123"},
        base::BindLambdaForTesting(
            [&](const base::flat_map<std::string, std::string>& result) {
              EXPECT_EQ(
                  result.find("0xf563fc08AFD59e2260b7d2d4481215c745Fa7573")
                      ->second,
                  "t410f6vr7ycfp2wpceyfx2lkeqeqvy5c7u5ltn7oripq");
              EXPECT_EQ(result.size(), 1u);
              callback_is_called = true;
            }));
    ASSERT_TRUE(callback_is_called);
  }
}

TEST_F(BraveWalletServiceUnitTest, GenerateReceiveAddress_EthFilSol) {
  SetupWallet();

  std::vector<mojom::AccountInfoPtr> accounts;
  accounts.push_back(GetAccountUtils().EnsureEthAccount(0));
  accounts.push_back(GetAccountUtils().EnsureSolAccount(0));
  accounts.push_back(GetAccountUtils().EnsureFilAccount(0));
  accounts.push_back(GetAccountUtils().EnsureFilTestAccount(0));

  for (auto& acc : accounts) {
    base::MockCallback<BraveWalletService::GenerateReceiveAddressCallback>
        callback;

    auto expected_address = std::optional<std::string>(acc->address);
    EXPECT_CALL(callback, Run(expected_address, std::optional<std::string>()))
        .Times(2);
    service_->GenerateReceiveAddress(acc->account_id.Clone(), callback.Get());
    service_->GenerateReceiveAddress(acc->account_id.Clone(), callback.Get());
    testing::Mock::VerifyAndClearExpectations(&callback);
  }
}

TEST_F(BraveWalletServiceUnitTest, GenerateReceiveAddress_Btc) {
  SetupWallet();

  auto btc_account = GetAccountUtils().EnsureBtcAccount(0);
  bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicDivideCruise, 0);

  base::MockCallback<BraveWalletService::GenerateReceiveAddressCallback>
      callback;

  std::optional<std::string> expected_address =
      keyring_service_
          ->GetBitcoinAddress(btc_account->account_id,
                              mojom::BitcoinKeyId::New(0, 1))
          ->address_string;

  EXPECT_CALL(callback, Run(expected_address, std::optional<std::string>()));
  service_->GenerateReceiveAddress(btc_account->account_id.Clone(),
                                   callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, Run(expected_address, std::optional<std::string>()));
  service_->GenerateReceiveAddress(btc_account->account_id.Clone(),
                                   callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  bitcoin_test_rpc_server_->address_stats_map()[*expected_address] =
      BitcoinTestRpcServer::TransactedAddressStats(*expected_address);

  expected_address = keyring_service_
                         ->GetBitcoinAddress(btc_account->account_id,
                                             mojom::BitcoinKeyId::New(0, 2))
                         ->address_string;
  EXPECT_CALL(callback, Run(expected_address, std::optional<std::string>()));
  service_->GenerateReceiveAddress(btc_account->account_id.Clone(),
                                   callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(BraveWalletServiceUnitTest, GetAnkrSupportedChainIds) {
  service_->GetAnkrSupportedChainIds(
      base::BindLambdaForTesting([](const std::vector<std::string>& chains) {
        std::vector<std::string> expected_chains = {
            mojom::kArbitrumMainnetChainId, mojom::kAvalancheMainnetChainId,
            mojom::kBaseMainnetChainId,     mojom::kBnbSmartChainMainnetChainId,
            mojom::kMainnetChainId,         mojom::kFantomMainnetChainId,
            mojom::kFlareMainnetChainId,    mojom::kGnosisChainId,
            mojom::kOptimismMainnetChainId, mojom::kPolygonMainnetChainId,
            mojom::kPolygonZKEVMChainId,    mojom::kRolluxMainnetChainId,
            mojom::kSyscoinMainnetChainId,  mojom::kZkSyncEraChainId};
        EXPECT_THAT(chains,
                    testing::UnorderedElementsAreArray(expected_chains));
      }));
}

TEST_F(BraveWalletServiceUnitTest, MaybeMigrateCompressedNfts) {
  GetPrefs()->SetBoolean(kBraveWalletIsCompressedNftMigrated, false);

  std::vector<mojom::BlockchainTokenPtr> tokens;
  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_FALSE(tokens[0]->is_nft);

  // Add a compressed Solana NFT that's not marked as compressed before
  // migration.
  auto nft = mojom::BlockchainToken::New();
  nft->contract_address = "AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW";
  nft->name = "Solana NFT";
  nft->logo = "solana.png";
  nft->is_compressed = false;
  nft->is_erc20 = false;
  nft->is_erc721 = false;
  nft->is_erc1155 = false;
  nft->is_nft = true;
  nft->is_spam = false;
  nft->symbol = "SOLNFT";
  nft->decimals = 0;
  nft->visible = true;
  nft->chain_id = mojom::kSolanaMainnet;
  nft->coin = mojom::CoinType::SOL;

  // Add it, but mock simple hash response saying it's not compressed
  std::map<GURL, std::string> responses;
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW")] =
      R"({
    "nfts": [
      {
        "nft_id": "solana.AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "chain": "solana",
        "contract_address": "AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator standing with his two back legs, big wings that make him move and attack quickly, and his tail like a big sword that can easily cut-off enemies into slices.",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "contract": {
          "type": "NonFungibleEdition",
          "name": "Common Water Warrior #19",
          "symbol": "DRAGON",
          "deployed_by": null,
          "deployed_via_contract": null,
          "owned_by": null,
          "has_multiple_collections": false
        },
        "collection": {
          "collection_id": "2732df34e18c360ccc0cc0809177c70b",
          "name": null,
          "description": null,
          "image_url": "https://lh3.googleusercontent.com/WXQW8GJiTDlucKnaip3NJC_4iFvLCfbQ_Ep9y4D7x-ElE5jOMlKJwcyqD7v27M7yPNiHlIxq9clPqylLlQVoeNfFvmXqboUPhDsS",
          "spam_score": 73
        },
        "last_sale": null,
        "first_created": {},
        "rarity": {
          "rank": null,
          "score": null,
          "unique_attributes": null
        },
        "royalty": [],
        "extra_metadata": {
          "token_program": "BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY"
        }
      }
    ]
  })";
  SetInterceptors(responses);
  ASSERT_TRUE(AddUserAsset(nft.Clone()));

  // Add non compressed solana NFT
  auto nft2 = mojom::BlockchainToken::New();
  nft2->contract_address = "BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW";
  nft2->name = "Solana NFT 2";
  nft2->logo = "solana2.png";
  nft2->is_compressed = false;
  nft2->is_erc20 = false;
  nft2->is_erc721 = false;
  nft2->is_erc1155 = false;
  nft2->is_nft = true;
  nft2->is_spam = false;
  nft2->symbol = "SOLNFT2";
  nft2->decimals = 0;
  nft2->visible = true;
  nft2->chain_id = mojom::kSolanaMainnet;
  nft2->coin = mojom::CoinType::SOL;

  // Add it, but mock simple hash response saying it's not compressed
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW")] =
      R"({
    "nfts": [
      {
        "nft_id": "solana.BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "chain": "solana",
        "contract_address": "BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator standing with his two back legs, big wings that make him move and attack quickly, and his tail like a big sword that can easily cut-off enemies into slices.",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "contract": {
          "type": "NonFungibleEdition",
          "name": "Common Water Warrior #19",
          "symbol": "DRAGON",
          "deployed_by": null,
          "deployed_via_contract": null,
          "owned_by": null,
          "has_multiple_collections": false
        },
        "collection": {
          "collection_id": "2732df34e18c360ccc0cc0809177c70b",
          "name": null,
          "description": null,
          "image_url": "https://lh3.googleusercontent.com/WXQW8GJiTDlucKnaip3NJC_4iFvLCfbQ_Ep9y4D7x-ElE5jOMlKJwcyqD7v27M7yPNiHlIxq9clPqylLlQVoeNfFvmXqboUPhDsS",
          "spam_score": 73
        }
      }
    ]
  })";
  SetInterceptors(responses);
  ASSERT_TRUE(AddUserAsset(nft2.Clone()));

  // Check that it's added
  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_TRUE(tokens[1]->is_nft);
  EXPECT_TRUE(tokens[1]->contract_address ==
              "AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW");
  EXPECT_FALSE(tokens[1]->is_compressed);
  EXPECT_TRUE(tokens[2]->is_nft);
  EXPECT_TRUE(tokens[2]->contract_address ==
              "BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW");
  EXPECT_FALSE(tokens[2]->is_compressed);

  // Now mock the response saying it's compressed (note compression field).
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW%"
      "2Csolana.BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW")] =
      R"({
    "nfts": [
      {
        "nft_id": "solana.AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "chain": "solana",
        "contract_address": "AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator standing with his two back legs, big wings that make him move and attack quickly, and his tail like a big sword that can easily cut-off enemies into slices.",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "contract": {
          "type": "NonFungibleEdition",
          "name": "Common Water Warrior #19",
          "symbol": "DRAGON",
          "deployed_by": null,
          "deployed_via_contract": null,
          "owned_by": null,
          "has_multiple_collections": false
        },
        "collection": {
          "collection_id": "2732df34e18c360ccc0cc0809177c70b",
          "name": null,
          "description": null,
          "image_url": "https://lh3.googleusercontent.com/WXQW8GJiTDlucKnaip3NJC_4iFvLCfbQ_Ep9y4D7x-ElE5jOMlKJwcyqD7v27M7yPNiHlIxq9clPqylLlQVoeNfFvmXqboUPhDsS",
          "spam_score": 73
        },
        "last_sale": null,
        "first_created": {},
        "rarity": {
          "rank": null,
          "score": null,
          "unique_attributes": null
        },
        "royalty": [],
        "extra_metadata": {
          "compression": {
            "compressed": true,
            "merkle_tree": "7eFJyb6UF4hQS7nSQaiy8Xpdq6V7Q1ZRjD3Lze11DZTd",
            "leaf_index": 1316261
          },
          "token_program": "BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY"
        }
      },
      {
        "nft_id": "solana.BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "chain": "solana",
        "contract_address": "BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator standing with his two back legs, big wings that make him move and attack quickly, and his tail like a big sword that can easily cut-off enemies into slices.",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "contract": {
          "type": "NonFungibleEdition",
          "name": "Common Water Warrior #19",
          "symbol": "DRAGON",
          "deployed_by": null,
          "deployed_via_contract": null,
          "owned_by": null,
          "has_multiple_collections": false
        },
        "collection": {
          "collection_id": "2732df34e18c360ccc0cc0809177c70b",
          "name": null,
          "description": null,
          "image_url": "https://lh3.googleusercontent.com/WXQW8GJiTDlucKnaip3NJC_4iFvLCfbQ_Ep9y4D7x-ElE5jOMlKJwcyqD7v27M7yPNiHlIxq9clPqylLlQVoeNfFvmXqboUPhDsS",
          "spam_score": 73
        }
      }
    ]
  })";
  SetInterceptors(responses);

  // Reset kBraveWalletIsCompressedNftMigrated pref, and run the migration.
  service_->MaybeMigrateCompressedNfts();
  task_environment_.RunUntilIdle();

  // Check that the NFT is now compressed, and the other is not.
  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_TRUE(tokens[1]->contract_address ==
              "AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW");
  EXPECT_TRUE(tokens[1]->is_compressed);
  EXPECT_TRUE(tokens[2]->contract_address ==
              "BM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW");
  EXPECT_FALSE(tokens[2]->is_compressed);
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletIsCompressedNftMigrated));
}

TEST_F(BraveWalletServiceUnitTest, MaybeMigrateSPLNfts) {
  GetPrefs()->SetBoolean(kBraveWalletIsSPLTokenProgramMigrated, false);

  std::vector<mojom::BlockchainTokenPtr> tokens;
  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_FALSE(tokens[0]->is_nft);

  // SPL NFT that's marked as kUnsupported SPL token program before migration
  // should be migrated to kUnknown.
  auto nft_unsupported = mojom::BlockchainToken::New();
  nft_unsupported->contract_address =
      "AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW";
  nft_unsupported->name = "Solana NFT";
  nft_unsupported->logo = "solana.png";
  nft_unsupported->is_compressed = false;
  nft_unsupported->is_erc20 = false;
  nft_unsupported->is_erc721 = false;
  nft_unsupported->is_erc1155 = false;
  nft_unsupported->is_nft = true;
  nft_unsupported->is_spam = false;
  nft_unsupported->symbol = "SOLNFT";
  nft_unsupported->decimals = 0;
  nft_unsupported->visible = true;
  nft_unsupported->chain_id = mojom::kSolanaMainnet;
  nft_unsupported->coin = mojom::CoinType::SOL;
  nft_unsupported->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
  auto added_nft_unsupported =
      ::brave_wallet::AddUserAsset(GetPrefs(), std::move(nft_unsupported));
  ASSERT_TRUE(added_nft_unsupported);

  // Non-NFT asset, should remain unchanged.
  auto non_nft = mojom::BlockchainToken::New();
  non_nft->contract_address = "F7E9L2tuxC9TQ6TMEFPNztefr9qiq6EnuJA1KgDWcpeZ";
  non_nft->name = "Solana Token";
  non_nft->logo = "solana_token.png";
  non_nft->is_compressed = false;
  non_nft->is_erc20 = true;
  non_nft->is_erc721 = false;
  non_nft->is_erc1155 = false;
  non_nft->is_nft = false;
  non_nft->is_spam = false;
  non_nft->symbol = "SOLTOKEN";
  non_nft->decimals = 0;
  non_nft->visible = true;
  non_nft->chain_id = mojom::kSolanaMainnet;
  non_nft->coin = mojom::CoinType::SOL;
  non_nft->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
  auto added_non_nft =
      ::brave_wallet::AddUserAsset(GetPrefs(), std::move(non_nft));
  ASSERT_TRUE(added_non_nft);

  // SPL NFT already marked with a different program (e.g., kUnknown), should
  // remain unchanged.
  auto nft_known_program = mojom::BlockchainToken::New();
  nft_known_program->contract_address =
      "B29EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeX";
  nft_known_program->name = "Known Program NFT";
  nft_known_program->logo = "known_program.png";
  nft_known_program->is_compressed = false;
  nft_known_program->is_erc20 = false;
  nft_known_program->is_erc721 = false;
  nft_known_program->is_erc1155 = false;
  nft_known_program->is_nft = true;
  nft_known_program->is_spam = false;
  nft_known_program->symbol = "KNOWNFT";
  nft_known_program->decimals = 0;
  nft_known_program->visible = true;
  nft_known_program->chain_id = mojom::kSolanaMainnet;
  nft_known_program->coin = mojom::CoinType::SOL;
  nft_known_program->spl_token_program = mojom::SPLTokenProgram::kUnknown;
  auto added_nft_known_program =
      ::brave_wallet::AddUserAsset(GetPrefs(), std::move(nft_known_program));
  ASSERT_TRUE(added_nft_known_program);

  // Case 4: Non-SOL NFT that should not be changed.
  auto non_sol_nft = mojom::BlockchainToken::New();
  non_sol_nft->contract_address = "0xAF5AD1e10926C0eE4aF4EDAc61Dd60E853753f8A";
  non_sol_nft->name = "Non-SOL NFT";
  non_sol_nft->logo = "non_sol_nft.png";
  non_sol_nft->is_compressed = false;
  non_sol_nft->is_erc20 = false;
  non_sol_nft->is_erc721 = true;
  non_sol_nft->is_erc1155 = false;
  non_sol_nft->is_nft = true;
  non_sol_nft->is_spam = false;
  non_sol_nft->symbol = "NONSOLNFT";
  non_sol_nft->decimals = 0;
  non_sol_nft->visible = true;
  non_sol_nft->chain_id = mojom::kMainnetChainId;
  non_sol_nft->coin = mojom::CoinType::ETH;
  non_sol_nft->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
  non_sol_nft->token_id = "0x1";
  auto added_non_sol_nft =
      ::brave_wallet::AddUserAsset(GetPrefs(), std::move(non_sol_nft));
  ASSERT_TRUE(added_non_sol_nft);

  // Run the migration.
  service_->MaybeMigrateSPLTokenProgram();
  task_environment_.RunUntilIdle();

  // Verify that the NFT marked as kUnsupported is now set to kUnknown.
  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  EXPECT_EQ(tokens.size(), 4u);  // Initial token + 3 added assets on Solana
  EXPECT_EQ(tokens[1]->contract_address,
            "AM1EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeW");
  EXPECT_TRUE(tokens[1]->is_nft);
  EXPECT_EQ(tokens[1]->spl_token_program, mojom::SPLTokenProgram::kUnknown);

  // Verify that the non-NFT token's SPLTokenProgram remains unchanged.
  EXPECT_EQ(tokens[2]->contract_address,
            "F7E9L2tuxC9TQ6TMEFPNztefr9qiq6EnuJA1KgDWcpeZ");
  EXPECT_FALSE(tokens[2]->is_nft);
  EXPECT_EQ(tokens[2]->spl_token_program, mojom::SPLTokenProgram::kUnsupported);

  // Verify that the NFT already with a known program remains unchanged.
  EXPECT_EQ(tokens[3]->contract_address,
            "B29EG2tuxB8TS6HMwEPNztegr9qio5EyuJA1KgDWcpeX");
  EXPECT_TRUE(tokens[3]->is_nft);
  EXPECT_EQ(tokens[3]->spl_token_program, mojom::SPLTokenProgram::kUnknown);

  // Verify that the non-SOL NFT remains unchanged.
  GetUserAssets(mojom::kMainnetChainId, mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[2]->contract_address,
            "0xAF5AD1e10926C0eE4aF4EDAc61Dd60E853753f8A");
  EXPECT_TRUE(tokens[2]->is_nft);
  EXPECT_EQ(tokens[2]->spl_token_program, mojom::SPLTokenProgram::kUnsupported);

  // Migration should be marked as done.
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletIsSPLTokenProgramMigrated));
}

TEST_F(BraveWalletServiceUnitTest, GetCountryCode) {
  const struct {
    const int country_code;
    const std::string expected_country;
  } kCountryCodeCases[] = {{21843, "US"}, {17217, "CA"}, {16725, "AU"}};

  for (const auto& [country_code, expected_country] : kCountryCodeCases) {
    GetPrefs()->SetInteger(country_codes::kCountryIDAtInstall, country_code);
    service_->GetCountryCode(base::BindLambdaForTesting(
        [&expected_country](const std::string& cc) -> void {
          EXPECT_EQ(expected_country, cc);
        }));
  }
}

}  // namespace brave_wallet
