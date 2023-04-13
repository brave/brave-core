/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include <memory>
#include <string>

#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
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
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

using content::StoragePartition;

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

const char goerli_list_json[] = R"(
  {
   "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
    "name": "USD Coin",
    "logo": "usdc.png",
    "erc20": true,
    "erc721": false,
    "symbol": "USDC",
    "decimals": 6,
    "chainId": "0x5"
   },
   "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
     "name": "Crypto Kitties",
     "logo": "CryptoKitties-Kitty-13733.svg",
     "erc20": false,
     "erc721": true,
     "symbol": "CK",
     "decimals": 0,
     "chainId": "0x5"
   },
   "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
     "name": "Uniswap",
     "logo": "uni.svg",
     "erc20": true,
     "symbol": "UNI",
     "decimals": 18,
     "chainId": "0x5"
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
        features::kNativeBraveWalletFeature);

#if BUILDFLAG(IS_ANDROID)
    task_environment_.AdvanceClock(base::Days(2));
#else
    base::Time future_mock_time;
    if (base::Time::FromString("3000-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }
#endif

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    histogram_tester_ = std::make_unique<base::HistogramTester>();
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_ =
        JsonRpcServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    tx_service = TxServiceFactory::GetServiceForContext(profile_.get());
    service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(profile_.get()), keyring_service_,
        json_rpc_service_, tx_service, GetPrefs(), local_state_->Get());
    observer_ = std::make_unique<TestBraveWalletServiceObserver>();
    service_->AddObserver(observer_->GetReceiver());

    auto* registry = BlockchainRegistry::GetInstance();
    TokenListMap token_list_map;
    ASSERT_TRUE(
        ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
    ASSERT_TRUE(ParseTokenList(goerli_list_json, &token_list_map,
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
    eth_token_->is_erc20 = false;
    eth_token_->is_erc721 = false;
    eth_token_->is_erc1155 = false;
    eth_token_->is_nft = false;
    eth_token_->decimals = 18;
    eth_token_->visible = true;
    eth_token_->chain_id = "0x1";
    eth_token_->coin = mojom::CoinType::ETH;

    bat_token_ = mojom::BlockchainToken::New();
    bat_token_->contract_address = "0x0D8775F648430679A709E98d2b0Cb6250d2887EF";
    bat_token_->name = "Basic Attention Token";
    bat_token_->symbol = "BAT";
    bat_token_->is_erc20 = true;
    bat_token_->is_erc721 = false;
    bat_token_->is_erc1155 = false;
    bat_token_->decimals = 18;
    bat_token_->visible = true;
    bat_token_->logo = "bat.png";
    bat_token_->chain_id = "0x1";
    bat_token_->coin = mojom::CoinType::ETH;

    sol_token_ = mojom::BlockchainToken::New(
        "", "Solana", "sol.png", false, false, false, false, "SOL", 9, true, "",
        "", mojom::kSolanaMainnet, mojom::CoinType::SOL);
    fil_token_ = mojom::BlockchainToken::New(
        "", "Filecoin", "fil.png", false, false, false, false, "FIL", 18, true,
        "", "", mojom::kFilecoinMainnet, mojom::CoinType::FIL);
  }

  mojom::BlockchainTokenPtr GetToken1() { return token1_.Clone(); }
  mojom::BlockchainTokenPtr GetToken2() { return token2_.Clone(); }
  mojom::BlockchainTokenPtr GetErc721Token() { return erc721_token_.Clone(); }
  mojom::BlockchainTokenPtr GetEthToken() { return eth_token_.Clone(); }
  mojom::BlockchainTokenPtr GetBatToken() { return bat_token_.Clone(); }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return brave_wallet::GetNetworkURL(GetPrefs(), chain_id, coin);
  }

  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  BlockchainRegistry* GetRegistry() {
    return BlockchainRegistry::GetInstance();
  }

  void SetGetEthNftStandardInterceptor(
      const GURL& expected_url,
      const std::map<std::string, std::string>& interface_id_to_response) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_url,
         interface_id_to_response](const network::ResourceRequest& request) {
          EXPECT_EQ(request.url, expected_url);
          base::StringPiece request_string(request.request_body->elements()
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

  void AddUserAsset(mojom::BlockchainTokenPtr token, bool* out_success) {
    base::RunLoop run_loop;
    service_->AddUserAsset(std::move(token),
                           base::BindLambdaForTesting([&](bool success) {
                             *out_success = success;
                             run_loop.Quit();
                           }));
    run_loop.Run();
  }

  void RemoveUserAsset(mojom::BlockchainTokenPtr token, bool* out_success) {
    base::RunLoop run_loop;
    service_->RemoveUserAsset(std::move(token),
                              base::BindLambdaForTesting([&](bool success) {
                                *out_success = success;
                                run_loop.Quit();
                              }));
  }

  void SetUserAssetVisible(mojom::BlockchainTokenPtr token,
                           bool visible,
                           bool* out_success) {
    base::RunLoop run_loop;
    service_->SetUserAssetVisible(std::move(token), visible,
                                  base::BindLambdaForTesting([&](bool success) {
                                    *out_success = success;
                                    run_loop.Quit();
                                  }));
    run_loop.Run();
  }

  void SetDefaultEthereumWallet(mojom::DefaultWallet default_wallet) {
    auto old_default_wallet = observer_->GetDefaultEthereumWallet();
    EXPECT_FALSE(observer_->DefaultEthereumWalletChangedFired());
    service_->SetDefaultEthereumWallet(default_wallet);
    base::RunLoop().RunUntilIdle();
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
    base::RunLoop().RunUntilIdle();
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
    base::RunLoop().RunUntilIdle();
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
    base::RunLoop().RunUntilIdle();
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

  mojom::CoinType GetSelectedCoin() {
    base::RunLoop run_loop;
    mojom::CoinType selected_coin;
    service_->GetSelectedCoin(
        base::BindLambdaForTesting([&](mojom::CoinType coin_type) {
          selected_coin = coin_type;
          run_loop.Quit();
        }));
    run_loop.Run();
    return selected_coin;
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
            [&](bool success,
                const absl::optional<std::string>& error_message) {
              *success_out = success;
              if (error_message)
                *error_message_out = *error_message;
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
          for (const auto& request : requests)
            requests_out.push_back(request.Clone());
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
    keyring_service_->GetMnemonicForDefaultKeyring(
        new_password,
        base::BindLambdaForTesting([&](const std::string& mnemonic) {
          *valid_mnemonic = (mnemonic == in_mnemonic);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void CheckAddresses(const std::vector<std::string>& addresses,
                      bool* valid_addresses) {
    ASSERT_NE(valid_addresses, nullptr);

    base::RunLoop run_loop;
    keyring_service_->GetKeyringInfo(
        brave_wallet::mojom::kDefaultKeyringId,
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          *valid_addresses = false;
          if (keyring_info->account_infos.size() == addresses.size()) {
            for (size_t i = 0; i < addresses.size(); ++i) {
              *valid_addresses =
                  (keyring_info->account_infos[i]->address == addresses[i]);
              if (!*valid_addresses)
                break;
            }
          }
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddSuggestToken(mojom::BlockchainTokenPtr suggested_token,
                       mojom::BlockchainTokenPtr expected_token,
                       bool approve,
                       bool run_switch_network = false) {
    mojom::AddSuggestTokenRequestPtr request =
        mojom::AddSuggestTokenRequest::New(
            MakeOriginInfo(url::Origin::Create(GURL("https://brave.com"))),
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
      json_rpc_service_->SetNetwork(mojom::kGoerliChainId,
                                    mojom::CoinType::ETH);
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
          for (const auto& request : requests)
            requests_out.push_back(request.Clone());
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

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<BraveWalletService> service_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  JsonRpcService* json_rpc_service_;
  TxService* tx_service;
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
  bool success = false;
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
  eth_0x5_token->chain_id = "0x5";

  // ETH should be returned before any token is added.
  GetUserAssets("0x5", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], eth_0x5_token);

  // Prepare tokens to add.
  mojom::BlockchainTokenPtr token1 = GetToken1();
  mojom::BlockchainTokenPtr token2 = GetToken2();

  // Add tokens and test GetUserAsset.
  AddUserAsset(token1.Clone(), &success);
  EXPECT_TRUE(success);

  // Adding token with lower case contract address should be converted to
  // checksum address.
  auto unchecked_token = token1.Clone();
  unchecked_token->chain_id = "0xaa36a7";
  unchecked_token->contract_address =
      base::ToLowerASCII(unchecked_token->contract_address);
  AddUserAsset(std::move(unchecked_token), &success);
  EXPECT_TRUE(success);

  auto token2_0xaa36a7 = token2.Clone();
  token2_0xaa36a7->chain_id = "0xaa36a7";
  AddUserAsset(token2_0xaa36a7.Clone(), &success);
  EXPECT_TRUE(success);

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
  RemoveUserAsset(token1_0x1.Clone(), &success);
  EXPECT_TRUE(success);

  RemoveUserAsset(token2_0xaa36a7.Clone(), &success);
  EXPECT_TRUE(success);

  GetUserAssets("0x1", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
  EXPECT_EQ(token1_0xaa36a7, tokens[1]);
}

TEST_F(BraveWalletServiceUnitTest, DefaultAssets) {
  mojom::BlockchainTokenPtr eth_token = GetEthToken();
  mojom::BlockchainTokenPtr bat_token = GetBatToken();

  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::ETH)) {
    auto native_asset = mojom::BlockchainToken::New(
        "", chain->symbol_name, "", false, false, false, false, chain->symbol,
        chain->decimals, true, "", "", chain->chain_id, mojom::CoinType::ETH);
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
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::SOL)) {
    SCOPED_TRACE(testing::PrintToString(chain->chain_id));
    std::vector<mojom::BlockchainTokenPtr> tokens;
    sol_token->chain_id = chain->chain_id;
    GetUserAssets(chain->chain_id, mojom::CoinType::SOL, &tokens);
    EXPECT_EQ(tokens.size(), 1u);
    EXPECT_EQ(sol_token, tokens[0]);
  }

  mojom::BlockchainTokenPtr fil_token = fil_token_->Clone();
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::FIL)) {
    SCOPED_TRACE(testing::PrintToString(chain->chain_id));
    std::vector<mojom::BlockchainTokenPtr> tokens;
    fil_token->chain_id = chain->chain_id;
    GetUserAssets(chain->chain_id, mojom::CoinType::FIL, &tokens);
    EXPECT_EQ(tokens.size(), 1u);
    EXPECT_EQ(fil_token, tokens[0]);
  }
}

TEST_F(BraveWalletServiceUnitTest, AddUserAsset) {
  bool success = false;
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
  AddUserAsset(std::move(token_with_empty_contract_address), &success);
  EXPECT_FALSE(success);

  // Invalid chain_id will fail.
  auto token_0x123 = token.Clone();
  token_0x123->chain_id = "0x123";
  AddUserAsset(std::move(token_0x123), &success);
  EXPECT_FALSE(success);

  // Add token.
  AddUserAsset(token.Clone(), &success);
  EXPECT_TRUE(success);

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
  AddUserAsset(token.Clone(), &success);
  EXPECT_FALSE(success);

  // Adding token with same address in lower cases in the same chain will fail.
  auto token_with_unchecked_address = token.Clone();
  token_with_unchecked_address->contract_address =
      base::ToLowerASCII(token->contract_address);
  AddUserAsset(token_with_unchecked_address.Clone(), &success);
  EXPECT_FALSE(success);

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
  AddUserAsset(token_with_unchecked_address.Clone(), &success);
  EXPECT_TRUE(success);

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], eth_0xaa36a7_token);
  EXPECT_EQ(tokens[1], token1_0xaa36a7);
}

TEST_F(BraveWalletServiceUnitTest, AddUserAssetNfts) {
  bool success = false;
  std::map<std::string, std::string> responses;
  std::vector<mojom::BlockchainTokenPtr> tokens;
  GURL network = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);

  // is_erc721 is set to true based on supportsInterface call results.
  mojom::BlockchainTokenPtr erc721_token = mojom::BlockchainToken::New(
      "0xBC4CA0EdA7647A8aB7C2061c2E118A18a936f13D", "BAYC", "bayc.png", false,
      false, false, true, "BAYC", 0, true, "0x1", "", mojom::kMainnetChainId,
      mojom::CoinType::ETH);
  responses[kERC721InterfaceId] = interface_supported_response;
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  AddUserAsset(erc721_token.Clone(), &success);
  EXPECT_TRUE(success);
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
      false, false, false, true, "ADIDAS", 0, true, "0x1", "",
      mojom::kMainnetChainId, mojom::CoinType::ETH);
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  AddUserAsset(erc1155.Clone(), &success);
  EXPECT_TRUE(success);
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
      false, false, false, true, "ADIDAS", 0, true, "0x2", "",
      mojom::kMainnetChainId, mojom::CoinType::ETH);
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  AddUserAsset(erc1155_2.Clone(), &success);
  EXPECT_TRUE(success);
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
      false, false, false, true, "333333", 0, true, "0x1", "",
      mojom::kMainnetChainId, mojom::CoinType::ETH);
  responses[kERC721InterfaceId] = "invalid";
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  AddUserAsset(erc1155_3.Clone(), &success);
  EXPECT_FALSE(success);

  // If neither erc721 nor erc1155 is supported, AddUserAsset returns false.
  mojom::BlockchainTokenPtr erc1155_4 = mojom::BlockchainToken::New(
      "0x4444444444444444444444444444444444444444", "444444", "444444.png",
      false, false, false, true, "444444", 0, true, "0x1", "",
      mojom::kMainnetChainId, mojom::CoinType::ETH);
  responses[kERC721InterfaceId] = interface_not_supported_response;
  responses[kERC1155InterfaceId] = interface_not_supported_response;
  SetGetEthNftStandardInterceptor(network, responses);
  AddUserAsset(erc1155_4.Clone(), &success);
  EXPECT_FALSE(success);
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

  bool success = false;
  std::vector<mojom::BlockchainTokenPtr> tokens;

  // Add tokens
  AddUserAsset(token1.Clone(), &success);
  EXPECT_TRUE(success);

  AddUserAsset(token2.Clone(), &success);
  EXPECT_TRUE(success);

  AddUserAsset(token2_0xaa36a7.Clone(), &success);
  EXPECT_TRUE(success);

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
  RemoveUserAsset(std::move(invalid_eth_token), &success);
  EXPECT_FALSE(success);

  // Remove token with invalid network_id returns false.
  auto token1_0x123 = token1.Clone();
  token1_0x123->chain_id = "0x123";
  RemoveUserAsset(std::move(token1_0x123), &success);
  EXPECT_FALSE(success);

  // Returns false when we cannot find the list with network_id.
  auto token1_0x7 = token1.Clone();
  token1_0x7->chain_id = "0x7";
  RemoveUserAsset(std::move(token1_0x7), &success);
  EXPECT_FALSE(success);

  // Remove non-exist token returns true.
  auto token1_0xaa36a7 = token1.Clone();
  token1_0xaa36a7->chain_id = "0xaa36a7";
  RemoveUserAsset(std::move(token1_0xaa36a7), &success);
  EXPECT_TRUE(success);

  // Remove existing token.
  RemoveUserAsset(token2.Clone(), &success);
  EXPECT_TRUE(success);

  // Lowercase address will be converted to checksum address when removing
  // token.
  auto BAT_lower_case_addr = GetBatToken();
  BAT_lower_case_addr->contract_address =
      base::ToLowerASCII(BAT_lower_case_addr->contract_address);
  RemoveUserAsset(std::move(BAT_lower_case_addr), &success);
  EXPECT_TRUE(success);

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

  bool success = false;
  std::vector<mojom::BlockchainTokenPtr> tokens;

  // Add tokens
  AddUserAsset(token1.Clone(), &success);
  EXPECT_TRUE(success);

  AddUserAsset(token2.Clone(), &success);
  EXPECT_TRUE(success);

  AddUserAsset(token2_0xaa36a7.Clone(), &success);
  EXPECT_TRUE(success);

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
  SetUserAssetVisible(std::move(invalid_eth), false, &success);
  EXPECT_FALSE(success);

  // Invalid chain_id return false.
  auto token1_0x123 = token1.Clone();
  token1_0x123->chain_id = "0x123";
  SetUserAssetVisible(std::move(token1_0x123), false, &success);
  EXPECT_FALSE(success);

  // List for this network_id is not existed should return false.
  auto token1_0x5 = token1.Clone();
  token1_0x5->chain_id = "0x5";
  SetUserAssetVisible(std::move(token1_0x5), false, &success);
  EXPECT_FALSE(success);

  auto token1_0xaa36a7 = token1.Clone();
  token1_0xaa36a7->chain_id = "0xaa36a7";
  // No entry with this contract address exists in the list.
  SetUserAssetVisible(token1_0xaa36a7.Clone(), false, &success);
  EXPECT_FALSE(success);

  // Set visible to false for BAT & token1 in "0x1" and token2 in "0xaa36a7".
  SetUserAssetVisible(token1.Clone(), false, &success);
  EXPECT_TRUE(success);

  // Lowercase address will be converted to checksum address directly.
  auto BAT_lower_case_addr = GetBatToken();
  BAT_lower_case_addr->contract_address =
      base::ToLowerASCII(BAT_lower_case_addr->contract_address);
  SetUserAssetVisible(std::move(BAT_lower_case_addr), false, &success);
  EXPECT_TRUE(success);

  SetUserAssetVisible(token2_0xaa36a7.Clone(), false, &success);
  EXPECT_TRUE(success);

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

TEST_F(BraveWalletServiceUnitTest, GetChecksumAddress) {
  absl::optional<std::string> addr = service_->GetChecksumAddress(
      "0x06012c8cf97bead5deae237070f9587f8e7a266d", "0x1");
  EXPECT_EQ(addr.value(), "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");

  addr = service_->GetChecksumAddress(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1");
  EXPECT_EQ(addr.value(), "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");

  addr = service_->GetChecksumAddress("", "0x1");
  EXPECT_EQ(addr.value(), "");

  addr = service_->GetChecksumAddress("eth", "0x1");
  EXPECT_FALSE(addr.has_value());

  addr = service_->GetChecksumAddress("ETH", "0x1");
  EXPECT_FALSE(addr.has_value());

  addr = service_->GetChecksumAddress("0x123", "0x1");
  EXPECT_FALSE(addr.has_value());

  addr = service_->GetChecksumAddress("123", "0x1");
  EXPECT_FALSE(addr.has_value());

  addr = service_->GetChecksumAddress(
      "06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1");
  EXPECT_FALSE(addr.has_value());
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

TEST_F(BraveWalletServiceUnitTest, SelectedCoin) {
  EXPECT_EQ(static_cast<int>(mojom::CoinType::ETH),
            GetPrefs()->GetInteger(kBraveWalletSelectedCoin));
  EXPECT_EQ(mojom::CoinType::ETH, GetSelectedCoin());

  service_->SetSelectedCoin(mojom::CoinType::SOL);
  EXPECT_EQ(static_cast<int>(mojom::CoinType::SOL),
            GetPrefs()->GetInteger(kBraveWalletSelectedCoin));
  EXPECT_EQ(mojom::CoinType::SOL, GetSelectedCoin());
}

TEST_F(BraveWalletServiceUnitTest, EthAddRemoveSetUserAssetVisible) {
  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";
  bool success = false;
  std::vector<mojom::BlockchainTokenPtr> tokens;

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);

  // Add ETH again will fail.
  AddUserAsset(eth_0xaa36a7_token.Clone(), &success);
  EXPECT_FALSE(success);

  // Test setting visibility of ETH.
  SetUserAssetVisible(eth_0xaa36a7_token.Clone(), false, &success);
  EXPECT_TRUE(success);

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_FALSE(tokens[0]->visible);

  // Test removing ETH from user asset list.
  RemoveUserAsset(eth_0xaa36a7_token.Clone(), &success);
  EXPECT_TRUE(success);

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_TRUE(tokens.empty());

  // Add ETH with eth as the contract address will fail.
  auto invalid_eth = eth_0xaa36a7_token.Clone();
  invalid_eth->contract_address = "eth";
  AddUserAsset(std::move(invalid_eth), &success);
  EXPECT_FALSE(success);

  // Add ETH with empty contract address.
  AddUserAsset(eth_0xaa36a7_token.Clone(), &success);
  EXPECT_TRUE(success);

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
}

TEST_F(BraveWalletServiceUnitTest, NetworkListChangedEvent) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x5566");

  AddCustomNetwork(GetPrefs(), chain);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer_->OnNetworkListChangedFired());

  // Remove network.
  observer_->Reset();
  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletCustomNetworks);
    base::Value::List* list = update->FindList(kEthereumPrefKey);
    list->EraseIf([&](const base::Value& v) {
      auto* chain_id_value = v.GetDict().FindString("chainId");
      if (!chain_id_value)
        return false;
      return *chain_id_value == "0x5566";
    });
  }
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer_->OnNetworkListChangedFired());
}

TEST_F(BraveWalletServiceUnitTest,
       CustomChainNativeAssetAddRemoveSetUserAssetVisible) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x5566");
  AddCustomNetwork(GetPrefs(), chain);

  auto native_asset = mojom::BlockchainToken::New(
      "", "symbol_name", "https://url1.com", false, false, false, false,
      "symbol", 11, true, "", "", "0x5566", mojom::CoinType::ETH);

  bool success = false;
  std::vector<mojom::BlockchainTokenPtr> tokens;

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(native_asset.Clone(), tokens[0]);

  // Add native asset again will fail.
  AddUserAsset(native_asset.Clone(), &success);
  EXPECT_FALSE(success);

  // Test setting visibility of ETH.
  SetUserAssetVisible(native_asset.Clone(), false, &success);
  EXPECT_TRUE(success);

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_FALSE(tokens[0]->visible);

  // Test removing native asset from user asset list.
  RemoveUserAsset(native_asset.Clone(), &success);
  EXPECT_TRUE(success);

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_TRUE(tokens.empty());

  // Add native asset again
  AddUserAsset(native_asset.Clone(), &success);
  EXPECT_TRUE(success);

  GetUserAssets("0x5566", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(native_asset.Clone(), tokens[0]);
}

TEST_F(BraveWalletServiceUnitTest, ERC721TokenAddRemoveSetUserAssetVisible) {
  bool success = false;
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
  AddUserAsset(std::move(erc721_token_with_empty_token_id), &success);
  EXPECT_FALSE(success);

  // Add ERC721 token with token_id = 1 should success.
  AddUserAsset(erc721_token_1.Clone(), &success);
  EXPECT_TRUE(success);

  // Add the same token_id should fail.
  AddUserAsset(erc721_token_1.Clone(), &success);
  EXPECT_FALSE(success);

  // Add to another chain should success
  auto erc721_token_1_0x1 = erc721_token_1.Clone();
  erc721_token_1_0x1->chain_id = "0x1";
  network = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  SetGetEthNftStandardInterceptor(network, responses);
  AddUserAsset(erc721_token_1_0x1.Clone(), &success);
  EXPECT_TRUE(success);

  // Add ERC721 token with token_id = 2 should success.
  network = GetNetwork(mojom::kSepoliaChainId, mojom::CoinType::ETH);
  SetGetEthNftStandardInterceptor(network, responses);
  AddUserAsset(erc721_token_2.Clone(), &success);
  EXPECT_TRUE(success);

  mojom::BlockchainTokenPtr eth_0xaa36a7_token = GetEthToken();
  eth_0xaa36a7_token->chain_id = "0xaa36a7";

  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
  EXPECT_EQ(erc721_token_1, tokens[1]);
  EXPECT_EQ(erc721_token_2, tokens[2]);

  SetUserAssetVisible(erc721_token_1.Clone(), false, &success);
  EXPECT_TRUE(success);

  RemoveUserAsset(erc721_token_2.Clone(), &success);
  EXPECT_TRUE(success);

  auto erc721_token_1_visible_false = erc721_token_1.Clone();
  erc721_token_1_visible_false->visible = false;
  GetUserAssets("0xaa36a7", mojom::CoinType::ETH, &tokens);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(eth_0xaa36a7_token, tokens[0]);
  EXPECT_EQ(erc721_token_1_visible_false, tokens[1]);
}

TEST_F(BraveWalletServiceUnitTest, SolanaTokenUserAssetsAPI) {
  bool success = false;
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
  AddUserAsset(sol_usdc_->Clone(), &success);
  EXPECT_TRUE(success);
  auto wrapped_sol_devnet = wrapped_sol_->Clone();
  wrapped_sol_devnet->chain_id = mojom::kSolanaDevnet;
  AddUserAsset(wrapped_sol_devnet->Clone(), &success);
  EXPECT_TRUE(success);

  GetUserAssets(mojom::kSolanaMainnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(sol_usdc_, tokens[1]);

  GetUserAssets(mojom::kSolanaDevnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 2u);
  EXPECT_EQ(wrapped_sol_devnet, tokens[1]);

  // Set visible of wrapped sol to false on devnet
  EXPECT_TRUE(tokens[1]->visible);
  SetUserAssetVisible(wrapped_sol_devnet->Clone(), false, &success);
  ASSERT_TRUE(success);
  GetUserAssets(mojom::kSolanaDevnet, mojom::CoinType::SOL, &tokens);
  ASSERT_EQ(tokens.size(), 2u);
  auto non_visible_wrapped_sol_devnet = wrapped_sol_devnet->Clone();
  non_visible_wrapped_sol_devnet->visible = false;
  EXPECT_EQ(non_visible_wrapped_sol_devnet, tokens[1]);

  // Remove usdc from mainnet and wrapped sol from devnet.
  RemoveUserAsset(sol_usdc_->Clone(), &success);
  RemoveUserAsset(wrapped_sol_devnet->Clone(), &success);
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
  AddUserAsset(sol_0x100->Clone(), &success);
  EXPECT_FALSE(success);
  RemoveUserAsset(sol_0x100->Clone(), &success);
  EXPECT_FALSE(success);
  SetUserAssetVisible(sol_0x100->Clone(), true, &success);
  EXPECT_FALSE(success);
}

TEST_F(BraveWalletServiceUnitTest, MigrateUserAssetsDefaultPrefs) {
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsDeprecated));
  BraveWalletService::MigrateUserAssetEthContractAddress(GetPrefs());
  BraveWalletService::MigrateMultichainUserAssets(GetPrefs());
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsDeprecated));
  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssets));
}

TEST_F(BraveWalletServiceUnitTest, MigrateUserAssetEthContractAddress) {
  EXPECT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated));

  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletUserAssetsDeprecated);

    base::Value::List user_assets_list;
    base::Value::Dict value;
    value.Set("contract_address", "eth");
    value.Set("name", "Ethereum");
    value.Set("symbol", "ETH");
    value.Set("is_erc20", false);
    value.Set("is_erc721", false);
    value.Set("is_erc1155", false);
    value.Set("decimals", 18);
    value.Set("visible", true);
    user_assets_list.Append(std::move(value));

    update->Set("goerli", std::move(user_assets_list));
  }

  const auto& pref = GetPrefs()->GetDict(kBraveWalletUserAssetsDeprecated);
  const auto* user_assets_list = pref.FindList("goerli");
  ASSERT_TRUE(user_assets_list);
  ASSERT_EQ(user_assets_list->size(), 1u);
  EXPECT_EQ(*(*user_assets_list)[0].GetDict().FindString("contract_address"),
            "eth");

  BraveWalletService::MigrateUserAssetEthContractAddress(GetPrefs());
  ASSERT_EQ(user_assets_list->size(), 1u);
  EXPECT_EQ(*(*user_assets_list)[0].GetDict().FindString("contract_address"),
            "");

  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated));
}

TEST_F(BraveWalletServiceUnitTest, MigrateMultichainUserAssets) {
  ASSERT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsDeprecated));

  {
    ScopedDictPrefUpdate update(GetPrefs(), kBraveWalletUserAssetsDeprecated);
    auto& old_user_assets_pref = update.Get();

    base::Value::Dict value;
    value.Set("contract_address", "");
    value.Set("name", "Ethereum");
    value.Set("symbol", "ETH");
    value.Set("is_erc20", false);
    value.Set("is_erc721", false);
    value.Set("is_erc1155", false);
    value.Set("decimals", 18);
    value.Set("visible", true);
    base::Value::List mainnet_user_assets_list;
    mainnet_user_assets_list.Append(std::move(value));

    base::Value::Dict value2;
    value2.Set("contract_address",
               "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
    value2.Set("name", "Basic Attention Token");
    value2.Set("symbol", "BAT");
    value2.Set("is_erc20", true);
    value2.Set("is_erc721", false);
    value2.Set("is_erc1155", false);
    value2.Set("decimals", 18);
    value2.Set("visible", true);
    mainnet_user_assets_list.Append(std::move(value2));

    base::Value::Dict value3;
    value3.Set("contract_address", "");
    value3.Set("name", "Ethereum");
    value3.Set("symbol", "ETH");
    value3.Set("is_erc20", false);
    value3.Set("is_erc721", false);
    value3.Set("is_erc1155", false);
    value3.Set("decimals", 18);
    value3.Set("visible", true);
    base::Value::List rinkbey_user_assets_list;
    rinkbey_user_assets_list.Append(std::move(value3));

    old_user_assets_pref.Set("mainnet", std::move(mainnet_user_assets_list));
    old_user_assets_pref.Set("rinkbey", std::move(rinkbey_user_assets_list));
  }

  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsDeprecated));
  BraveWalletService::MigrateMultichainUserAssets(GetPrefs());

  const auto& assets = GetPrefs()->GetDict(kBraveWalletUserAssets);
  const auto* ethereum_mainnet_list =
      assets.FindListByDottedPath("ethereum.mainnet");
  ASSERT_TRUE(ethereum_mainnet_list);
  ASSERT_EQ(ethereum_mainnet_list->size(), 2u);
  EXPECT_FALSE(
      (*ethereum_mainnet_list)[0].GetDict().FindString("contract_address"));
  EXPECT_FALSE(
      (*ethereum_mainnet_list)[1].GetDict().FindString("contract_address"));
  EXPECT_EQ(*(*ethereum_mainnet_list)[0].GetDict().FindString("address"), "");
  EXPECT_EQ(*(*ethereum_mainnet_list)[1].GetDict().FindString("address"),
            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  const auto* ethereum_rinkbey_list =
      assets.FindListByDottedPath("ethereum.rinkbey");
  ASSERT_TRUE(ethereum_rinkbey_list);
  ASSERT_EQ(ethereum_rinkbey_list->size(), 1u);
  EXPECT_FALSE(
      (*ethereum_rinkbey_list)[0].GetDict().FindString("contract_address"));
  EXPECT_EQ(*(*ethereum_rinkbey_list)[0].GetDict().FindString("address"), "");

  const auto* solana_dict = assets.FindDict("solana");
  ASSERT_TRUE(solana_dict);
  EXPECT_EQ(*solana_dict, BraveWalletService::GetDefaultSolanaAssets());

  const auto* filecoin_dict = assets.FindDict("filecoin");
  ASSERT_TRUE(filecoin_dict);
  EXPECT_EQ(*filecoin_dict, BraveWalletService::GetDefaultFilecoinAssets());

  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssetsDeprecated));
}

TEST_F(BraveWalletServiceUnitTest, MigrateUserAssetsAddPreloadingNetworks) {
  ASSERT_FALSE(GetPrefs()->GetBoolean(
      kBraveWalletUserAssetsAddPreloadingNetworksMigrated));

  // Test cases covered:
  // 1. Network that has existing native asset -> no change.
  // 2. Network with existing custom tokens -> native asset should be inserted
  //    at first.
  // 3. Network with empty asset list -> should append native asset.
  std::string json = R"({
    "ethereum": {
      "mainnet": [
        {
          "address": "",
          "name": "Ethereum",
          "symbol": "ETH",
          "is_erc20": false,
          "is_erc721": false,
          "decimals": 18,
          "visible": false
        }
      ],
      "0xfa": [
        {
          "address":"0x6a31Aca4d2f7398F04d9B6ffae2D898d9A8e7938",
          "coingecko_id":"",
          "decimals":18,
          "is_erc20":true,
          "is_erc721":false,
          "logo":"https://brave.com/logo.jpg",
          "name":"WTRTL",
          "symbol":"WTRTL",
          "token_id":"",
          "visible":true
        }
      ],
      "0x89": []
    }
  })";
  auto user_assets_value = base::JSONReader::Read(json);
  ASSERT_TRUE(user_assets_value);
  GetPrefs()->Set(kBraveWalletUserAssets, *user_assets_value);

  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletUserAssets));
  BraveWalletService::MigrateUserAssetsAddPreloadingNetworks(GetPrefs());

  auto wtrtl = mojom::BlockchainToken::New(
      "0x6a31Aca4d2f7398F04d9B6ffae2D898d9A8e7938", "WTRTL",
      "https://brave.com/logo.jpg", true, false, false, false, "WTRTL", 18,
      true, "", "", mojom::kFantomMainnetChainId, mojom::CoinType::ETH);
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::ETH)) {
    auto native_asset = mojom::BlockchainToken::New(
        "", chain->symbol_name, "", false, false, false, false, chain->symbol,
        chain->decimals, true, "", "", chain->chain_id, mojom::CoinType::ETH);
    std::vector<mojom::BlockchainTokenPtr> tokens;
    GetUserAssets(chain->chain_id, mojom::CoinType::ETH, &tokens);

    if (chain->chain_id == mojom::kMainnetChainId)
      native_asset->visible = false;

    if (chain->chain_id == mojom::kFantomMainnetChainId) {
      EXPECT_EQ(tokens.size(), 2u);
      EXPECT_EQ(tokens[1], wtrtl);
    } else {
      EXPECT_EQ(tokens.size(), 1u);
    }
    EXPECT_EQ(tokens[0], native_asset);
  }

  EXPECT_TRUE(GetPrefs()->GetBoolean(
      kBraveWalletUserAssetsAddPreloadingNetworksMigrated));
}

TEST_F(BraveWalletServiceUnitTest, MigrateUserAssetsAddIsNFT) {
  ASSERT_FALSE(GetPrefs()->GetBoolean(kBraveWalletUserAssetsAddIsNFTMigrated));

  std::string json = R"({
    "ethereum": {
      "mainnet": [
        {
          "address": "",
          "name": "Ethereum",
          "symbol": "ETH",
          "is_erc20": false,
          "is_erc721": false,
          "decimals": 18,
          "visible": true
        },
        {
          "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
          "name": "Basic Attention Token",
          "symbol": "BAT",
          "is_erc20": true,
          "is_erc721": false,
          "decimals": 18,
          "visible": true
        },
        {
          "address": "0x0D8775F648430679A709E98d2b0Cb6250d288888",
          "name": "My NFT",
          "symbol": "MN",
          "is_erc20": false,
          "is_erc721": true,
          "token_id": 1,
          "visible": false
        }
      ],
      "0x89": [
        {
          "address": "",
          "coingecko_id": "",
          "decimals": 18,
          "is_erc20": false,
          "is_erc721": false,
          "logo": "https://brave.com/logo.jpg",
          "name": "MATIC",
          "symbol": "MATIC",
          "token_id": "",
          "visible": true
        }
      ]
    },
    "solana": {
      "mainnet": [
        {
          "address": "",
          "coingecko_id": "",
          "decimals": 9,
          "is_erc20": false,
          "is_erc721": false,
          "logo": "https://brave.com/logo.jpg",
          "name": "Solana",
          "symbol": "SOL",
          "visible": true
        }
      ]
    }
  })";
  auto user_assets_value = base::JSONReader::Read(json);
  ASSERT_TRUE(user_assets_value);
  GetPrefs()->Set(kBraveWalletUserAssets, *user_assets_value);
  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletUserAssets));
  BraveWalletService::MigrateUserAssetsAddIsNFT(GetPrefs());

  base::ReplaceSubstringsAfterOffset(&json, 0, "\"is_erc721\": false",
                                     R"("is_erc721": false, "is_nft": false)");
  base::ReplaceSubstringsAfterOffset(&json, 0, "\"is_erc721\": true",
                                     R"("is_erc721": true, "is_nft": true)");
  user_assets_value = base::JSONReader::Read(json);
  ASSERT_TRUE(user_assets_value);
  EXPECT_EQ(GetPrefs()->GetValue(kBraveWalletUserAssets), *user_assets_value);

  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletUserAssetsAddIsNFTMigrated));
}

TEST_F(BraveWalletServiceUnitTest, MigradeDefaultHiddenNetworks) {
  ASSERT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultHiddenNetworksVersion),
            0);
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
  RemoveHiddenNetwork(GetPrefs(), mojom::CoinType::ETH, "0x4cb2f");
  BraveWalletService::MigrateHiddenNetworks(GetPrefs());
  {
    auto* list =
        GetPrefs()->GetDict(kBraveWalletHiddenNetworks).FindList("ethereum");
    ASSERT_EQ(std::find_if(list->begin(), list->end(),
                           [](const auto& v) { return v == "0x4cb2f"; }),
              list->end());
  }
}

TEST_F(BraveWalletServiceUnitTest, MigradeDefaultHiddenNetworks_NoList) {
  ASSERT_EQ(GetPrefs()->GetInteger(kBraveWalletDefaultHiddenNetworksVersion),
            0);
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

TEST_F(BraveWalletServiceUnitTest, MigrateUserAssetsAddIsERC1155) {
  ASSERT_FALSE(
      GetPrefs()->GetBoolean(kBraveWalletUserAssetsAddIsERC1155Migrated));

  std::string json = R"({
    "ethereum": {
      "mainnet": [
        {
          "address": "",
          "name": "Ethereum",
          "symbol": "ETH",
          "is_erc20": false,
          "is_erc721": false,
          "decimals": 18,
          "visible": true
        },
        {
          "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
          "name": "Basic Attention Token",
          "symbol": "BAT",
          "is_erc20": true,
          "is_erc721": false,
          "decimals": 18,
          "visible": true
        },
        {
          "address": "0x0D8775F648430679A709E98d2b0Cb6250d288888",
          "name": "My NFT",
          "symbol": "MN",
          "is_erc20": false,
          "is_erc721": true,
          "token_id": 1,
          "visible": false
        }
      ],
      "0x89": [
        {
          "address": "",
          "coingecko_id": "",
          "decimals": 18,
          "is_erc20": false,
          "is_erc721": false,
          "logo": "https://brave.com/logo.jpg",
          "name": "MATIC",
          "symbol": "MATIC",
          "token_id": "",
          "visible": true
        }
      ]
    },
    "solana": {
      "mainnet": [
        {
          "address": "",
          "coingecko_id": "",
          "decimals": 9,
          "is_erc20": false,
          "is_erc721": false,
          "logo": "https://brave.com/logo.jpg",
          "name": "Solana",
          "symbol": "SOL",
          "visible": true
        }
      ]
    }
  })";

  auto user_assets_value = base::JSONReader::Read(json);
  ASSERT_TRUE(user_assets_value);
  GetPrefs()->Set(kBraveWalletUserAssets, *user_assets_value);
  ASSERT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletUserAssets));
  BraveWalletService::MigrateUserAssetsAddIsERC1155(GetPrefs());

  // Add `"is_erc1155": false` key/values to the expected kBraveWalletUserAssets
  // json after migrating.
  base::ReplaceSubstringsAfterOffset(
      &json, 0, "\"is_erc721\": false",
      R"("is_erc721": false, "is_erc1155": false)");
  base::ReplaceSubstringsAfterOffset(
      &json, 0, "\"is_erc721\": true",
      R"("is_erc721": true, "is_erc1155": false)");
  user_assets_value = base::JSONReader::Read(json);
  ASSERT_TRUE(user_assets_value);
  EXPECT_EQ(GetPrefs()->GetValue(kBraveWalletUserAssets), *user_assets_value);
  EXPECT_TRUE(
      GetPrefs()->GetBoolean(kBraveWalletUserAssetsAddIsERC1155Migrated));
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
  const char* valid_mnemonic =
      "drip caution abandon festival order clown oven regular absorb evidence "
      "crew where";
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

    bool is_valid_addresses = false;
    const std::vector<std::string> expected_addresses(
        {"0x084DCb94038af1715963F149079cE011C4B22961",
         "0xE60A2209372AF1049C4848B1bF0136258c35f268",
         "0xb41c52De621B42A3a186ae1e608073A546195C9C"});
    CheckAddresses(expected_addresses, &is_valid_addresses);
    EXPECT_TRUE(is_valid_addresses);
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

    bool is_valid_addresses = false;
    const std::vector<std::string> expected_addresses(
        {"0xea3C17c81E3baC3472d163b2c8b12ddDAa027874",
         "0xEc1BB5a4EC94dE9107222c103907CCC720fA3854",
         "0x8cb80Ef1d274ED215A4C08B31b77e5A813eD8Ea1",
         "0x3899D70A5D45368807E38Ef2c1EB5E4f07542e4f"});
    CheckAddresses(expected_addresses, &is_valid_addresses);
    EXPECT_TRUE(is_valid_addresses);
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
  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://brave.com")));
  std::string expected_signature = std::string("0xSiGnEd");
  std::string address = "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c";
  std::string domain = "{}";
  std::string message = "0xAB";
  auto request1 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 1, address, domain, message, false, absl::nullopt,
      absl::nullopt, absl::nullopt, mojom::CoinType::ETH);
  bool callback_is_called = false;
  service_->AddSignMessageRequest(
      std::move(request1),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::ByteArrayStringUnionPtr signature,
                                     const absl::optional<std::string>& error) {
        ASSERT_TRUE(approved);
        ASSERT_TRUE(signature->is_str());
        EXPECT_EQ(signature->get_str(), expected_signature);
        EXPECT_FALSE(error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(
      true, 1, mojom::ByteArrayStringUnion::NewStr(expected_signature),
      absl::nullopt);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  service_->NotifySignMessageRequestProcessed(
      true, 1, mojom::ByteArrayStringUnion::NewStr(expected_signature),
      absl::nullopt);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  callback_is_called = false;
  std::string expected_error = "error";
  auto request2 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 2, address, domain, message, false, absl::nullopt,
      absl::nullopt, absl::nullopt, mojom::CoinType::ETH);
  service_->AddSignMessageRequest(
      std::move(request2),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::ByteArrayStringUnionPtr signature,
                                     const absl::optional<std::string>& error) {
        ASSERT_FALSE(approved);
        ASSERT_TRUE(signature->is_str());
        EXPECT_EQ(signature->get_str(), expected_signature);
        ASSERT_TRUE(error);
        EXPECT_EQ(*error, expected_error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(
      false, 2, mojom::ByteArrayStringUnion::NewStr(expected_signature),
      expected_error);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
}

TEST_F(BraveWalletServiceUnitTest, SignMessage) {
  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://brave.com")));
  std::string expected_signature = std::string("0xSiGnEd");
  std::string address = "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c";
  std::string domain = "{}";
  std::string message = "0xAB";
  auto request1 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 1, address, domain, message, false, absl::nullopt,
      absl::nullopt, absl::nullopt, mojom::CoinType::ETH);
  bool callback_is_called = false;
  service_->AddSignMessageRequest(
      std::move(request1),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::ByteArrayStringUnionPtr signature,
                                     const absl::optional<std::string>& error) {
        ASSERT_TRUE(approved);
        EXPECT_FALSE(signature);
        EXPECT_FALSE(error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(true, 1, nullptr, absl::nullopt);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  service_->NotifySignMessageRequestProcessed(true, 1, nullptr, absl::nullopt);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
  callback_is_called = false;
  std::string expected_error = "error";
  auto request2 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 2, address, domain, message, false, absl::nullopt,
      absl::nullopt, absl::nullopt, mojom::CoinType::ETH);
  service_->AddSignMessageRequest(
      std::move(request2),
      base::BindLambdaForTesting([&](bool approved,
                                     mojom::ByteArrayStringUnionPtr signature,
                                     const absl::optional<std::string>& error) {
        ASSERT_FALSE(approved);
        EXPECT_FALSE(signature);
        EXPECT_FALSE(error);
        callback_is_called = true;
      }));
  EXPECT_EQ(GetPendingSignMessageRequests().size(), 1u);
  service_->NotifySignMessageRequestProcessed(false, 2, nullptr, absl::nullopt);
  ASSERT_TRUE(callback_is_called);
  ASSERT_TRUE(GetPendingSignMessageRequests().empty());
}

TEST_F(BraveWalletServiceUnitTest, AddSuggestToken) {
  std::vector<std::string> chain_ids = {mojom::kMainnetChainId,
                                        mojom::kGoerliChainId};
  for (const std::string& chain_id : chain_ids) {
    json_rpc_service_->SetNetwork(chain_id, mojom::CoinType::ETH);
    mojom::BlockchainTokenPtr usdc_from_blockchain_registry =
        mojom::BlockchainToken::New(
            "0x6B175474E89094C44Da98b954EedeAC495271d0F", "USD Coin",
            "usdc.png", true, false, false, false, "USDC", 6, true, "", "",
            chain_id, mojom::CoinType::ETH);
    ASSERT_EQ(usdc_from_blockchain_registry,
              GetRegistry()->GetTokenByAddress(
                  chain_id, mojom::CoinType::ETH,
                  "0x6B175474E89094C44Da98b954EedeAC495271d0F"));
    mojom::BlockchainTokenPtr usdc_from_user_assets =
        mojom::BlockchainToken::New(
            "0x6B175474E89094C44Da98b954EedeAC495271d0F", "USD Coin", "", true,
            false, false, false, "USDC", 6, true, "", "", chain_id,
            mojom::CoinType::ETH);
    ASSERT_TRUE(service_->AddUserAsset(usdc_from_user_assets.Clone()));

    mojom::BlockchainTokenPtr usdc_from_request = mojom::BlockchainToken::New(
        "0x6B175474E89094C44Da98b954EedeAC495271d0F", "USDC", "", true, false,
        false, false, "USDC", 6, true, "", "", chain_id, mojom::CoinType::ETH);

    mojom::BlockchainTokenPtr custom_token = mojom::BlockchainToken::New(
        "0x6b175474e89094C44Da98b954eEdeAC495271d1e", "COLOR", "", true, false,
        false, false, "COLOR", 18, true, "", "", chain_id,
        mojom::CoinType::ETH);

    // Case 1: Suggested token does not exist (no entry with the same contract
    // address) in BlockchainRegistry nor user assets.
    // Token should be in user asset list and is visible, and the data should be
    // the same as the one in the request.
    AddSuggestToken(custom_token.Clone(), custom_token.Clone(), true);
    auto token = service_->GetUserAsset(
        custom_token->contract_address, custom_token->token_id,
        custom_token->is_nft, chain_id, mojom::CoinType::ETH);
    EXPECT_EQ(token, custom_token);

    // Case 2: Suggested token exists (has an entry with the same contract
    // address) in BlockchainRegistry and user asset list and is visible.
    // Token should be in user asset list and is visible, and the data should be
    // the same as the one in the user asset list.
    AddSuggestToken(usdc_from_request.Clone(), usdc_from_user_assets.Clone(),
                    true);
    token = service_->GetUserAsset(usdc_from_user_assets->contract_address,
                                   usdc_from_user_assets->token_id,
                                   usdc_from_user_assets->is_nft, chain_id,
                                   mojom::CoinType::ETH);
    EXPECT_EQ(token, usdc_from_user_assets);

    // Case 3: Suggested token exists in BlockchainRegistry and user asset list
    // but is not visible. Token should be in user
    // asset list and is visible, and the data should be the same as the one in
    // the user asset list.
    ASSERT_TRUE(
        service_->SetUserAssetVisible(usdc_from_user_assets.Clone(), false));
    token = service_->GetUserAsset(usdc_from_user_assets->contract_address,
                                   usdc_from_user_assets->token_id,
                                   usdc_from_user_assets->is_nft, chain_id,
                                   mojom::CoinType::ETH);
    AddSuggestToken(usdc_from_request.Clone(), token.Clone(), true);
    token = service_->GetUserAsset(usdc_from_user_assets->contract_address,
                                   usdc_from_user_assets->token_id,
                                   usdc_from_user_assets->is_nft, chain_id,
                                   mojom::CoinType::ETH);
    EXPECT_EQ(token, usdc_from_user_assets);

    // Case 4: Suggested token exists in BlockchainRegistry but not in user
    // asset list. Token should be in user asset list and is visible, and the
    // data should be the same as the one in BlockchainRegistry.
    ASSERT_TRUE(service_->RemoveUserAsset(usdc_from_user_assets.Clone()));
    AddSuggestToken(usdc_from_request.Clone(),
                    usdc_from_blockchain_registry.Clone(), true);
    token = service_->GetUserAsset(
        usdc_from_blockchain_registry->contract_address,
        usdc_from_blockchain_registry->token_id,
        usdc_from_blockchain_registry->is_nft, chain_id, mojom::CoinType::ETH);
    EXPECT_EQ(token, usdc_from_blockchain_registry);

    mojom::BlockchainTokenPtr usdt_from_user_assets =
        mojom::BlockchainToken::New(
            "0xdAC17F958D2ee523a2206206994597C13D831ec7", "Tether", "usdt.png",
            true, false, false, false, "USDT", 6, true, "", "", chain_id,
            mojom::CoinType::ETH);
    ASSERT_TRUE(service_->AddUserAsset(usdt_from_user_assets.Clone()));

    mojom::BlockchainTokenPtr usdt_from_request = mojom::BlockchainToken::New(
        "0xdAC17F958D2ee523a2206206994597C13D831ec7", "USDT", "", true, false,
        false, false, "USDT", 18, true, "", "", chain_id, mojom::CoinType::ETH);
    // Case 5: Suggested token exists in user asset list and is visible, does
    // not exist in BlockchainRegistry. Token should be in user asset list and
    // is visible, and the data should be the same as the one in user asset
    // list.
    AddSuggestToken(usdt_from_request.Clone(), usdt_from_user_assets.Clone(),
                    true);
    token = service_->GetUserAsset(usdt_from_user_assets->contract_address,
                                   usdt_from_user_assets->token_id,
                                   usdt_from_user_assets->is_nft, chain_id,
                                   mojom::CoinType::ETH);
    EXPECT_EQ(token, usdt_from_user_assets);

    // Case 6: Suggested token exists in user asset list but is not visible,
    // does not exist in BlockchainRegistry. Token should be in user asset list
    // and is visible, and the data should be the same as the one in user asset
    // list.
    ASSERT_TRUE(
        service_->SetUserAssetVisible(usdt_from_user_assets.Clone(), false));
    token = service_->GetUserAsset(usdt_from_user_assets->contract_address,
                                   usdt_from_user_assets->token_id,
                                   usdt_from_user_assets->is_nft, chain_id,
                                   mojom::CoinType::ETH);
    AddSuggestToken(usdt_from_request.Clone(), token.Clone(), true);
    token = service_->GetUserAsset(usdt_from_user_assets->contract_address,
                                   usdt_from_user_assets->token_id,
                                   usdt_from_user_assets->is_nft, chain_id,
                                   mojom::CoinType::ETH);
    EXPECT_EQ(token, usdt_from_user_assets);

    // Call AddSuggestTokenRequest and switch network without
    // NotifyAddSuggestTokenRequestsProcessed being called should clear out the
    // pending request and AddSuggestTokenRequestCallback should be run with
    // kUserRejectedRequest error.
    mojom::BlockchainTokenPtr busd = mojom::BlockchainToken::New(
        "0x4Fabb145d64652a948d72533023f6E7A623C7C53", "Binance USD", "", true,
        false, false, false, "BUSD", 18, true, "", "", chain_id,
        mojom::CoinType::ETH);
    AddSuggestToken(busd.Clone(), busd.Clone(), false,
                    true /* run_switch_network */);

    // Test reject request.
    mojom::BlockchainTokenPtr brb_from_request = mojom::BlockchainToken::New(
        "0x6B175474E89094C44Da98b954EedeAC495271d0A", "BRB", "", true, false,
        false, false, "BRB", 6, true, "", "", chain_id, mojom::CoinType::ETH);
    ASSERT_TRUE(service_->RemoveUserAsset(brb_from_request.Clone()));
    AddSuggestToken(brb_from_request.Clone(), brb_from_request.Clone(), false);
    token = service_->GetUserAsset(
        brb_from_request->contract_address, brb_from_request->token_id,
        brb_from_request->is_nft, chain_id, mojom::CoinType::ETH);
    EXPECT_FALSE(token);
  }
}

TEST_F(BraveWalletServiceUnitTest, GetUserAsset) {
  mojom::BlockchainTokenPtr usdc = mojom::BlockchainToken::New(
      "0x6B175474E89094C44Da98b954EedeAC495271d0F", "USD Coin", "usdc.png",
      true, false, false, false, "USDC", 6, true, "", "", mojom::kGoerliChainId,
      mojom::CoinType::ETH);
  ASSERT_TRUE(service_->AddUserAsset(usdc.Clone()));
  EXPECT_EQ(usdc, service_->GetUserAsset(usdc->contract_address, usdc->token_id,
                                         usdc->is_nft, mojom::kGoerliChainId,
                                         mojom::CoinType::ETH));
  EXPECT_EQ(usdc,
            service_->GetUserAsset(
                base::ToLowerASCII(usdc->contract_address), usdc->token_id,
                usdc->is_nft, mojom::kGoerliChainId, mojom::CoinType::ETH));
  EXPECT_FALSE(service_->GetUserAsset(usdc->contract_address, usdc->token_id,
                                      usdc->is_nft, mojom::kMainnetChainId,
                                      mojom::CoinType::ETH));

  auto erc721_token_with_empty_token_id = GetErc721Token();
  auto erc721_token_1 = erc721_token_with_empty_token_id.Clone();
  erc721_token_1->token_id = "0x1";
  erc721_token_1->chain_id = mojom::kGoerliChainId;
  ASSERT_TRUE(service_->AddUserAsset(erc721_token_1.Clone()));
  EXPECT_EQ(
      erc721_token_1,
      service_->GetUserAsset(erc721_token_1->contract_address,
                             erc721_token_1->token_id, erc721_token_1->is_nft,
                             mojom::kGoerliChainId, mojom::CoinType::ETH));
  EXPECT_FALSE(service_->GetUserAsset(
      erc721_token_1->contract_address, "0x2", erc721_token_1->is_nft,
      mojom::kGoerliChainId, mojom::CoinType::ETH));
}

TEST_F(BraveWalletServiceUnitTest, Reset) {
  SetDefaultBaseCurrency("CAD");
  SetDefaultBaseCryptocurrency("ETH");
  mojom::BlockchainTokenPtr token1 = GetToken1();
  bool success;
  AddUserAsset(token1.Clone(), &success);
  EXPECT_TRUE(success);
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kBraveWalletUserAssets));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kDefaultBaseCurrency));
  EXPECT_TRUE(GetPrefs()->HasPrefPath(kDefaultBaseCryptocurrency));
  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://brave.com")));
  std::string address = "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c";
  std::string domain = "{}";
  std::string message = "0xAB";
  auto request1 = mojom::SignMessageRequest::New(
      origin_info.Clone(), 1, address, domain, message, false, absl::nullopt,
      absl::nullopt, absl::nullopt, mojom::CoinType::ETH);
  service_->AddSignMessageRequest(
      std::move(request1),
      base::BindLambdaForTesting([](bool, mojom::ByteArrayStringUnionPtr,
                                    const absl::optional<std::string>&) {}));
  mojom::BlockchainTokenPtr custom_token = mojom::BlockchainToken::New(
      "0x6b175474e89094C44Da98b954eEdeAC495271d1e", "COLOR", "", true, false,
      false, false, "COLOR", 18, true, "", "", "0x1", mojom::CoinType::ETH);
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

  service_->Reset();

  EXPECT_FALSE(GetPrefs()->HasPrefPath(kBraveWalletUserAssets));
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

TEST_F(BraveWalletServiceUnitTest, GetUserAssetAddress) {
  // Native asset
  EXPECT_EQ(
      *BraveWalletService::GetUserAssetAddress("", mojom::CoinType::ETH, "0x1"),
      "");
  EXPECT_EQ(*BraveWalletService::GetUserAssetAddress("", mojom::CoinType::SOL,
                                                     mojom::kSolanaMainnet),
            "");
  EXPECT_EQ(
      *BraveWalletService::GetUserAssetAddress("", mojom::CoinType::FIL, "f"),
      "");

  // ETH
  EXPECT_EQ(*BraveWalletService::GetUserAssetAddress(
                "0x6b175474e89094c44da98b954eedeac495271d0f",
                mojom::CoinType::ETH, "0x1"),
            "0x6B175474E89094C44Da98b954EedeAC495271d0F");

  // SOL
  EXPECT_EQ(*BraveWalletService::GetUserAssetAddress(
                "AQoKYV7tYpTrFZN6P5oUufbQKAUr9mNYGe1TTJC9wajM",
                mojom::CoinType::SOL, mojom::kSolanaMainnet),
            "AQoKYV7tYpTrFZN6P5oUufbQKAUr9mNYGe1TTJC9wajM");
  EXPECT_EQ(BraveWalletService::GetUserAssetAddress("not_base58_encoded_string",
                                                    mojom::CoinType::SOL,
                                                    mojom::kSolanaMainnet),
            absl::nullopt);

  // FIL
  EXPECT_EQ(BraveWalletService::GetUserAssetAddress(
                "f1h4n7rphclbmwyjcp6jrdiwlfcuwbroxy3jvg33q",
                mojom::CoinType::FIL, "f"),
            absl::nullopt);
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

  // Should be able to set to true
  service_->SetNftDiscoveryEnabled(true);
  EXPECT_TRUE(GetPrefs()->GetBoolean(kBraveWalletNftDiscoveryEnabled));

  // And then back to false
  service_->SetNftDiscoveryEnabled(false);
  EXPECT_FALSE(GetPrefs()->GetBoolean(kBraveWalletNftDiscoveryEnabled));
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

}  // namespace brave_wallet
