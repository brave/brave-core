/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include "base/base64.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// JSON RPC responses for eth_call to the BalanceScanner contract
const char eth_balance_detected_response[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000006e83695ab1f893c00"
  })";
const char eth_balance_not_detected_response[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000000"
  })";
const char eth_error_fetching_balance_response[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000000"
  })";

const char kMnemonic1[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";
const char kPasswordBrave[] = "brave";

const std::vector<std::string>& GetAssetDiscoverySupportedEthChainsForTest() {
  static base::NoDestructor<std::vector<std::string>>
      asset_discovery_supported_chains({mojom::kMainnetChainId,
                                        mojom::kPolygonMainnetChainId,
                                        mojom::kOptimismMainnetChainId});
  return *asset_discovery_supported_chains;
}

}  // namespace

class TestBraveWalletServiceObserverForAssetDiscovery
    : public brave_wallet::mojom::BraveWalletServiceObserver {
 public:
  TestBraveWalletServiceObserverForAssetDiscovery() = default;

  void OnDefaultEthereumWalletChanged(mojom::DefaultWallet wallet) override {}
  void OnDefaultSolanaWalletChanged(mojom::DefaultWallet wallet) override {}
  void OnActiveOriginChanged(mojom::OriginInfoPtr origin_info) override {}
  void OnDefaultBaseCurrencyChanged(const std::string& currency) override {}
  void OnDefaultBaseCryptocurrencyChanged(
      const std::string& cryptocurrency) override {}
  void OnNetworkListChanged() override {}
  void OnDiscoverAssetsCompleted(
      std::vector<mojom::BlockchainTokenPtr> discovered_assets) override {
    ASSERT_EQ(expected_contract_addresses_.size(), discovered_assets.size());
    for (size_t i = 0; i < discovered_assets.size(); i++) {
      EXPECT_EQ(expected_contract_addresses_[i],
                discovered_assets[i]->contract_address);
    }
    on_discover_assets_completed_fired_ = true;
    run_loop_asset_discovery_->Quit();
  }

  void WaitForOnDiscoverAssetsCompleted(
      const std::vector<std::string>& addresses) {
    expected_contract_addresses_ = addresses;
    run_loop_asset_discovery_ = std::make_unique<base::RunLoop>();
    run_loop_asset_discovery_->Run();
  }

  bool OnDiscoverAssetsCompletedFired() {
    return on_discover_assets_completed_fired_;
  }

  mojo::PendingRemote<brave_wallet::mojom::BraveWalletServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }
  void Reset() {
    expected_contract_addresses_.clear();
    on_discover_assets_completed_fired_ = false;
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop_asset_discovery_;
  std::vector<std::string> expected_contract_addresses_;
  bool on_discover_assets_completed_fired_ = false;
  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      observer_receiver_{this};
};

class AssetDiscoveryManagerUnitTest : public testing::Test {
 public:
  AssetDiscoveryManagerUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)),
        task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~AssetDiscoveryManagerUnitTest() override = default;

 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(
        features::kNativeBraveWalletFeature);

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_ =
        JsonRpcServiceFactory::GetServiceForContext(profile_.get());
    json_rpc_service_->SetAPIRequestHelperForTesting(
        shared_url_loader_factory_);
    tx_service = TxServiceFactory::GetServiceForContext(profile_.get());
    wallet_service_ = std::make_unique<BraveWalletService>(
        BraveWalletServiceDelegate::Create(profile_.get()), keyring_service_,
        json_rpc_service_, tx_service, GetPrefs(), GetLocalState());
    asset_discovery_manager_ = std::make_unique<AssetDiscoveryManager>(
        wallet_service_.get(), json_rpc_service_, keyring_service_, GetPrefs());
    asset_discovery_manager_->SetSupportedChainsForTesting(
        GetAssetDiscoverySupportedEthChainsForTest());
    wallet_service_observer_ =
        std::make_unique<TestBraveWalletServiceObserverForAssetDiscovery>();
    wallet_service_->AddObserver(wallet_service_observer_->GetReceiver());
  }

  void SetInterceptor(const GURL& intended_url, const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, intended_url, content](const network::ResourceRequest& request) {
          if (request.url.spec() == intended_url) {
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), content);
          }
        }));
  }

  // SetInterceptorForDiscoverSolAssets takes a map of addresses to responses
  // and adds the response if the address if found in the request string
  void SetInterceptorForDiscoverSolAssets(
      const GURL& intended_url,
      const std::map<std::string, std::string>& requests) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, intended_url, requests](const network::ResourceRequest& request) {
          if (request.url.spec() == intended_url) {
            base::StringPiece request_string(
                request.request_body->elements()
                    ->at(0)
                    .As<network::DataElementBytes>()
                    .AsStringPiece());
            std::string response;
            for (auto const& [key, val] : requests) {
              if (request_string.find(key) != std::string::npos) {
                response = val;
                break;
              }
            }
            ASSERT_FALSE(response.empty());
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  // Interceptor that takes a mapping of URLs to a mapping of addresses
  // (std::strings) to responses (std::strings). If the requested URL is one in
  // the map and the requested address is one in the map, the response is
  // returned.
  void SetInterceptorForDiscoverEthAssets(
      const std::map<GURL, std::map<std::string, std::string>>& requests) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, requests](const network::ResourceRequest& request) {
          for (auto const& [url, address_response_map] : requests) {
            if (request.url.spec() == url.spec()) {
              base::StringPiece request_string(
                  request.request_body->elements()
                      ->at(0)
                      .As<network::DataElementBytes>()
                      .AsStringPiece());
              std::string response;
              for (auto const& [address, potential_response] :
                   address_response_map) {
                // Trim leading "0x" from address before searching for it in the
                // it in the request string since it's not included in the
                // calldata
                if (request_string.find(address.substr(2)) !=
                    std::string::npos) {
                  response = potential_response;
                  break;
                }
              }
              ASSERT_FALSE(response.empty());
              url_loader_factory_.ClearResponses();
              url_loader_factory_.AddResponse(request.url.spec(), response);
            }
          }
        }));
  }

  void SetLimitExceededJsonErrorResponse() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(),
                                          R"({
            "jsonrpc":"2.0",
            "id":1,
            "error": {
              "code":-32005,
              "message": "Request exceeds defined limit"
            }
          })");
        }));
  }

  void TestDiscoverSolAssets(
      const std::vector<std::string>& account_addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    asset_discovery_manager_->remaining_buckets_ = 1;
    asset_discovery_manager_->DiscoverSolAssets(account_addresses, false);
    wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
        expected_token_contract_addresses);
    wallet_service_observer_->Reset();
  }

  void TestDiscoverEthAssets(
      const std::vector<std::string>& account_addresses,
      bool triggered_by_accounts_added,
      const std::vector<std::string>& expected_token_contract_addresses) {
    asset_discovery_manager_->remaining_buckets_ = 1;
    asset_discovery_manager_->DiscoverEthAssets(account_addresses,
                                                triggered_by_accounts_added);
    wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
        expected_token_contract_addresses);
    wallet_service_observer_->Reset();
  }

  void TestDiscoverAssetsOnAllSupportedChainsAccountsAddedV2(
      mojom::CoinType coin,
      const std::vector<std::string>& account_addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    base::Time last_discovered_assets_at =
        GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::vector<mojom::BlockchainTokenPtr>
                    discovered_assets) {
              ASSERT_EQ(discovered_assets.size(),
                        expected_token_contract_addresses.size());
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(discovered_assets[i]->contract_address,
                          expected_token_contract_addresses[i]);
              }
            }));
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChainsAccountsAdded(
        coin, account_addresses);
    base::RunLoop().RunUntilIdle();

    // Verify kBraveWalletLastDiscoveredAssetsAt prefs are not updated
    EXPECT_EQ(last_discovered_assets_at,
              GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt));

    // Verify observer event not fired
    EXPECT_FALSE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
    wallet_service_observer_->Reset();
  }

  void TestDiscoverAssetsOnAllSupportedChainsRefresh(
      std::map<mojom::CoinType, std::vector<std::string>>& addresses,
      base::OnceCallback<void(base::Time previous, base::Time current)>
          assets_last_discovered_at_test_fn,
      const std::vector<std::string>& expected_token_contract_addresses) {
    // Capture the previous value for kBraveWalletLastDiscoveredAssetsAt before
    // calling DiscoverAssetsOnAllSupportedChains
    const base::Time previous_assets_last_discovered_at =
        GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);

    // Call DiscoverAssetsOnAllSupportedChains
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChainsRefresh(
        addresses);

    // Wait for the the wallet service event to be emitted (meaning asset
    // discovery has totally completed)
    wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
        expected_token_contract_addresses);

    // Fetch the current value for kBraveWalletLastDiscoveredAssetsAt after
    // calling DiscoverAssetsOnAllSupportedChains and compare it against the
    // previous value using the provided test function
    base::Time current_assets_last_discovered_at =
        GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
    std::move(assets_last_discovered_at_test_fn)
        .Run(previous_assets_last_discovered_at,
             current_assets_last_discovered_at);
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return brave_wallet::GetNetworkURL(GetPrefs(), chain_id, coin);
  }

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<TestBraveWalletServiceObserverForAssetDiscovery>
      wallet_service_observer_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveWalletService> wallet_service_;
  std::unique_ptr<AssetDiscoveryManager> asset_discovery_manager_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  JsonRpcService* json_rpc_service_;
  TxService* tx_service;
  base::test::ScopedFeatureList scoped_feature_list_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(AssetDiscoveryManagerUnitTest,
       DiscoverAssetsOnAllSupportedChainsAccountsAddedV2) {
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;

  // Ethereum
  asset_discovery_manager_->SetSupportedChainsForTesting(
      {mojom::kMainnetChainId, mojom::kPolygonMainnetChainId});
  std::string token_list_json = R"({
      "0x3333333333333333333333333333333333333333": {
        "name": "3333",
        "logo": "333.svg",
        "erc20": true,
        "symbol": "333",
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
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  std::map<GURL, std::map<std::string, std::string>> requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_not_detected_response},
       }},
      {GetNetwork(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_not_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverAssetsOnAllSupportedChainsAccountsAddedV2(
      mojom::CoinType::ETH,
      {
          "0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
          "0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
      },
      {
          "0x3333333333333333333333333333333333333333",
          "0x4444444444444444444444444444444444444444",
      });

  // Solana
  const std::string first_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "l/QUsV2gleWOBK7DT7McygX06DWutQjr6AinX510aVU2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQluoAsOSueAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 0
          },
          "pubkey": "BhZnyGAe58uHHdFQgej8ShuDGy9JL1tbs29Bqx3FRgy1"
        },
        {
          "account": {
            "data": [
              "iOBUDkpieWsUu53IBhROGzPicXkIYV2OIGUzsFvIlvU2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugDkC1QCAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 0
          },
          "pubkey": "3Ra8B4XsnumedGgKvfussaTLxrhyxFAqMkGmst8UqX3k"
        }
      ]
    },
    "id": 1
  })";

  // Owner 8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB has mints
  // 7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs and
  // 4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g
  const std::string second_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "O0NuqIea7HUvvwtwGehJ95pVsBSH6xpS3rvbymg9TMNuN8HO+P8En+NLC+JfUEsxJnxEYiI50JuYlZKuo/DnTAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "56iYTYcGgVj3kQ1eTApSp9BJRAvjNfZ7AFbdyeKfGPLK"
        },
        {
          "account": {
            "data": [
              "ZuUYihMIoduQttMfP73KjD3yZ4yBEt/dPRksWjzEV6huN8HO+P8En+NLC+JfUEsxJnxEYiI50JuYlZKuo/DnTCeWHwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "dUomT6JpMrZioLeLgtLfcUQpqHsA2jiH9vvz8HDsbyZ"
        }
      ]
    },
    "id": 1
  })";

  const std::map<std::string, std::string> solana_requests = {
      {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF", first_response},
      {"8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB", second_response},
  };
  SetInterceptorForDiscoverSolAssets(
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL), solana_requests);
  // Add BEARs6toGY6fRGsmz2Se8NDuR2NVPRmJuLPpeF8YxCq2,
  // ADJqxHJRfFBpyxVQ2YS8nBhfW6dumdDYGU21B4AmYLZJ,
  // 7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs, and
  // 4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g to token list
  token_list_json = R"({
    "BEARs6toGY6fRGsmz2Se8NDuR2NVPRmJuLPpeF8YxCq2": {
      "name": "Tesla Inc.",
      "logo": "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
      "erc20": false,
      "symbol": "TSLA",
      "decimals": 8,
      "chainId": "0x65"
    },
    "ADJqxHJRfFBpyxVQ2YS8nBhfW6dumdDYGU21B4AmYLZJ": {
      "name": "Apple Inc.",
      "logo": "8bpRdBGPt354VfABL5xugP3pmYZ2tQjzRcqjg2kmwfbF.png",
      "erc20": false,
      "symbol": "AAPL",
      "decimals": 8,
      "chainId": "0x65"
    },
    "7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs": {
      "name": "Microsoft Corporation",
      "logo": "3vhcrQfEn8ashuBfE82F3MtEDFcBCEFfFw1ZgM3xj1s8.png",
      "erc20": false,
      "symbol": "MSFT",
      "decimals": 8,
      "chainId": "0x65"
    },
    "4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g": {
      "name": "MicroStrategy Incorporated.",
      "logo": "ASwYCbLedk85mRdPnkzrUXbbYbwe26m71af9rzrhC2Qz.png",
      "erc20": false,
      "symbol": "MSTR",
      "decimals": 8,
      "chainId": "0x65"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::SOL));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  TestDiscoverAssetsOnAllSupportedChainsAccountsAddedV2(
      mojom::CoinType::SOL,
      {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF",
       "8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB"},
      {"4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g",
       "7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs",
       "ADJqxHJRfFBpyxVQ2YS8nBhfW6dumdDYGU21B4AmYLZJ",
       "BEARs6toGY6fRGsmz2Se8NDuR2NVPRmJuLPpeF8YxCq2"});
}

TEST_F(AssetDiscoveryManagerUnitTest,
       DiscoverAssetsOnAllSupportedChainsRefresh) {
  std::string token_list_json;
  std::map<mojom::CoinType, std::vector<std::string>> addresses;
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  std::map<GURL, std::map<std::string, std::string>> requests;

  // Verify that in a single call, we can discover assets on multiple Ethereum
  // chains as well as Solana. Rate limit pref is updated.
  asset_discovery_manager_->SetSupportedChainsForTesting(
      {mojom::kMainnetChainId, mojom::kPolygonMainnetChainId});
  // Parse the ETH token list
  token_list_json = R"({
      "0x3333333333333333333333333333333333333333": {
        "name": "3333",
        "logo": "333.svg",
        "erc20": true,
        "symbol": "333",
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
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // Now parse the SOL token list
  token_list_json = R"({
    "EybFzCH4nBYEr7FD4wLWBvNZbEGgjy4kh584bGQntr1b": {
      "name": "USD Coin",
      "logo": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png",
      "erc20": false,
      "symbol": "USDC",
      "decimals": 6,
      "chainId": "0x65",
      "coingeckoId": "usd-coin"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::SOL));
  for (auto& list_pair : token_list_map) {
    blockchain_registry->UpdateTokenList(list_pair.first,
                                         std::move(list_pair.second));
  }

  std::string sol_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "z6cxAUoRHIupvmezOL4EAsTLlwKTgwxzCg/xcNWSEu42kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "5gjGaTE41sPVS1Dzwg43ipdj9NTtApZLcK55ihRuVb6Y"
        }
      ]
    },
    "id": 1
  })";

  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_not_detected_response},
       }},
      {GetNetwork(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_not_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_detected_response},
       }},
      {GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL),
       {
           {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF", sol_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  addresses = {
      {mojom::CoinType::ETH,
       {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
        "0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"}},
      {mojom::CoinType::SOL, {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"}},
  };
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_GT(current, previous);
      }),
      {"EybFzCH4nBYEr7FD4wLWBvNZbEGgjy4kh584bGQntr1b",
       "0x3333333333333333333333333333333333333333",
       "0x4444444444444444444444444444444444444444"});

  // Verify that subsequent calls are rate limited.
  // Need to adds some new assets to the token list first though
  token_list_json = R"({
      "0x5555555555555555555555555555555555555555": {
        "name": "3333",
        "logo": "333.svg",
        "erc20": true,
        "symbol": "333",
        "decimals": 18,
        "chainId": "0x1"
      }
     })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  // Now parse the SOL token list
  token_list_json = R"({
    "88j24JNwWLmJCjn2tZQ5jJzyaFtnusS2qsKup9NeDnd8": {
      "name": "Wrapped SOL",
      "logo": "So11111111111111111111111111111111111111112.png",
      "erc20": false,
      "symbol": "SOL",
      "decimals": 9,
      "chainId": "0x65",
      "coingeckoId": "solana"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::SOL));
  for (auto& list_pair : token_list_map) {
    blockchain_registry->UpdateTokenList(list_pair.first,
                                         std::move(list_pair.second));
  }

  sol_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "81ZdQjbr7FhEPmcyGJtG8BAUyWxAjb2iSiWFEQn8i8Da"
        }
      ]
    },
    "id": 1
  })";

  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_not_detected_response},
       }},
      {GetNetwork(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_not_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_detected_response},
       }},
      {GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL),
       {
           {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF", sol_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_EQ(current, previous);
      }),
      {});

  // Verify that after fast forwarding, we can discover assets again
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_GT(current, previous);
      }),
      {"88j24JNwWLmJCjn2tZQ5jJzyaFtnusS2qsKup9NeDnd8",
       "0x5555555555555555555555555555555555555555"});
}

TEST_F(AssetDiscoveryManagerUnitTest, KeyringServiceObserver) {
  // Verifies that the AssetDiscoveryManager is added as an observer to the
  // KeyringService, and that discovery is run when new accounts are added
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  std::string token_list_json = R"({
    "0x6b175474e89094c44da98b954eedeac495271d0f":{
      "name":"Dai Stablecoin",
      "logo":"dai.svg",
      "erc20":true,
      "symbol":"DAI",
      "decimals":18,
      "chainId":"0x1"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // RestoreWallet (restores 0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db)
  asset_discovery_manager_->SetSupportedChainsForTesting(
      {mojom::kMainnetChainId});
  std::vector<mojom::BlockchainTokenPtr> user_assets_before =
      BraveWalletService::GetUserAssets(mojom::kMainnetChainId,
                                        mojom::CoinType::ETH, GetPrefs());
  SetInterceptor(GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
                 eth_balance_detected_response);
  keyring_service_->RestoreWallet(kMnemonic1, kPasswordBrave, false,
                                  base::DoNothing());
  base::RunLoop().RunUntilIdle();
  std::vector<mojom::AccountInfoPtr> account_infos =
      keyring_service_->GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  std::vector<mojom::BlockchainTokenPtr> user_assets_after =
      BraveWalletService::GetUserAssets(mojom::kMainnetChainId,
                                        mojom::CoinType::ETH, GetPrefs());
  EXPECT_EQ(user_assets_after.size(), user_assets_before.size() + 1);
  EXPECT_EQ(user_assets_after[user_assets_after.size() - 1]->symbol, "DAI");
}

TEST_F(AssetDiscoveryManagerUnitTest, DecodeMintAddress) {
  // Invalid (data too short)
  absl::optional<std::vector<uint8_t>> data_short = base::Base64Decode("YQ==");
  ASSERT_TRUE(data_short);
  absl::optional<SolanaAddress> mint_address =
      asset_discovery_manager_->DecodeMintAddress(*data_short);
  ASSERT_FALSE(mint_address);

  // Valid
  absl::optional<std::vector<uint8_t>> data = base::Base64Decode(
      "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+"
      "lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/"
      "qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAA");
  ASSERT_TRUE(data);
  mint_address = asset_discovery_manager_->DecodeMintAddress(*data);
  ASSERT_TRUE(mint_address);
  EXPECT_EQ(mint_address.value().ToBase58(),
            "88j24JNwWLmJCjn2tZQ5jJzyaFtnusS2qsKup9NeDnd8");
}

TEST_F(AssetDiscoveryManagerUnitTest, DiscoverSolAssets) {
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  std::string token_list_json = R"({
    "88j24JNwWLmJCjn2tZQ5jJzyaFtnusS2qsKup9NeDnd8": {
      "name": "Wrapped SOL",
      "logo": "So11111111111111111111111111111111111111112.png",
      "erc20": false,
      "symbol": "SOL",
      "decimals": 9,
      "chainId": "0x65",
      "coingeckoId": "solana"
    },
    "EybFzCH4nBYEr7FD4wLWBvNZbEGgjy4kh584bGQntr1b": {
      "name": "USD Coin",
      "logo": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png",
      "erc20": false,
      "symbol": "USDC",
      "decimals": 6,
      "chainId": "0x65",
      "coingeckoId": "usd-coin"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::SOL));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // Empy account address
  TestDiscoverSolAssets({}, {});

  // Invalid
  TestDiscoverSolAssets({"ABC"}, {});

  // Empty response (no tokens found) yields success
  auto expected_network_url =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  SetInterceptor(expected_network_url, R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 171155478
      },
      "value": []
    },
    "id": 1
  })");
  TestDiscoverSolAssets({"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"}, {});

  // Invalid response (no tokens found) yields
  SetLimitExceededJsonErrorResponse();
  TestDiscoverSolAssets({"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"}, {});

  // Valid response containing both tokens should add both tokens
  SetInterceptor(expected_network_url, R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "z6cxAUoRHIupvmezOL4EAsTLlwKTgwxzCg/xcNWSEu42kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "5gjGaTE41sPVS1Dzwg43ipdj9NTtApZLcK55ihRuVb6Y"
        },
        {
          "account": {
            "data": [
              "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "81ZdQjbr7FhEPmcyGJtG8BAUyWxAjb2iSiWFEQn8i8Da"
        }
      ]
    },
    "id": 1
  })");
  TestDiscoverSolAssets({"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"},
                        {"88j24JNwWLmJCjn2tZQ5jJzyaFtnusS2qsKup9NeDnd8",
                         "EybFzCH4nBYEr7FD4wLWBvNZbEGgjy4kh584bGQntr1b"});

  // Making the same call again should not add any tokens (they've already been
  // added)
  TestDiscoverSolAssets({"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"}, {});

  // Should merge tokens from multiple accounts
  // (4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF and
  // 8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB) Owner
  // 4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF has mints
  // BEARs6toGY6fRGsmz2Se8NDuR2NVPRmJuLPpeF8YxCq2 and
  // ADJqxHJRfFBpyxVQ2YS8nBhfW6dumdDYGU21B4AmYLZJ
  const std::string first_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "l/QUsV2gleWOBK7DT7McygX06DWutQjr6AinX510aVU2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQluoAsOSueAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 0
          },
          "pubkey": "BhZnyGAe58uHHdFQgej8ShuDGy9JL1tbs29Bqx3FRgy1"
        },
        {
          "account": {
            "data": [
              "iOBUDkpieWsUu53IBhROGzPicXkIYV2OIGUzsFvIlvU2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugDkC1QCAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 0
          },
          "pubkey": "3Ra8B4XsnumedGgKvfussaTLxrhyxFAqMkGmst8UqX3k"
        }
      ]
    },
    "id": 1
  })";

  // Owner 8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB has mints
  // 7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs and
  // 4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g
  const std::string second_response = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.13.5",
        "slot": 166895942
      },
      "value": [
        {
          "account": {
            "data": [
              "O0NuqIea7HUvvwtwGehJ95pVsBSH6xpS3rvbymg9TMNuN8HO+P8En+NLC+JfUEsxJnxEYiI50JuYlZKuo/DnTAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "56iYTYcGgVj3kQ1eTApSp9BJRAvjNfZ7AFbdyeKfGPLK"
        },
        {
          "account": {
            "data": [
              "ZuUYihMIoduQttMfP73KjD3yZ4yBEt/dPRksWjzEV6huN8HO+P8En+NLC+JfUEsxJnxEYiI50JuYlZKuo/DnTCeWHwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": 2039280,
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": 361
          },
          "pubkey": "dUomT6JpMrZioLeLgtLfcUQpqHsA2jiH9vvz8HDsbyZ"
        }
      ]
    },
    "id": 1
  })";

  const std::map<std::string, std::string> requests = {
      {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF", first_response},
      {"8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB", second_response},
  };
  SetInterceptorForDiscoverSolAssets(expected_network_url, requests);

  // Add BEARs6toGY6fRGsmz2Se8NDuR2NVPRmJuLPpeF8YxCq2,
  // ADJqxHJRfFBpyxVQ2YS8nBhfW6dumdDYGU21B4AmYLZJ,
  // 7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs, and
  // 4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g to token list
  token_list_json = R"({
    "BEARs6toGY6fRGsmz2Se8NDuR2NVPRmJuLPpeF8YxCq2": {
      "name": "Tesla Inc.",
      "logo": "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
      "erc20": false,
      "symbol": "TSLA",
      "decimals": 8,
      "chainId": "0x65"
    },
    "ADJqxHJRfFBpyxVQ2YS8nBhfW6dumdDYGU21B4AmYLZJ": {
      "name": "Apple Inc.",
      "logo": "8bpRdBGPt354VfABL5xugP3pmYZ2tQjzRcqjg2kmwfbF.png",
      "erc20": false,
      "symbol": "AAPL",
      "decimals": 8,
      "chainId": "0x65"
    },
    "7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs": {
      "name": "Microsoft Corporation",
      "logo": "3vhcrQfEn8ashuBfE82F3MtEDFcBCEFfFw1ZgM3xj1s8.png",
      "erc20": false,
      "symbol": "MSFT",
      "decimals": 8,
      "chainId": "0x65"
    },
    "4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g": {
      "name": "MicroStrategy Incorporated.",
      "logo": "ASwYCbLedk85mRdPnkzrUXbbYbwe26m71af9rzrhC2Qz.png",
      "erc20": false,
      "symbol": "MSTR",
      "decimals": 8,
      "chainId": "0x65"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::SOL));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  TestDiscoverSolAssets({"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF",
                         "8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB"},
                        {"4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g",
                         "7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs",
                         "ADJqxHJRfFBpyxVQ2YS8nBhfW6dumdDYGU21B4AmYLZJ",
                         "BEARs6toGY6fRGsmz2Se8NDuR2NVPRmJuLPpeF8YxCq2"});
}

TEST_F(AssetDiscoveryManagerUnitTest, DiscoverEthAssets) {
  // Supplying no addresses should end early but still trigger an
  // OnDiscoverAssetsCompleted event
  TestDiscoverEthAssets({}, false, {});

  // Add token to the registry for upcoming tests
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  std::string token_list_json = R"({
     "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
       "name": "Dai Stablecoin",
       "logo": "dai.svg",
       "erc20": true,
       "symbol": "DAI",
       "chainId": "0x1",
       "decimals": 18
     }
    })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // One account, no balances, yields empty token_contract_addresses
  asset_discovery_manager_->SetSupportedChainsForTesting(
      {mojom::kMainnetChainId});
  std::map<GURL, std::map<std::string, std::string>> requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_not_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, false,
                        {});

  // One account, BalanceScanner reports failure to fetch (successfull), yields
  // no discovered contract addresses
  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_error_fetching_balance_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, false,
                        {});

  // One account returns the cUSDT token response for no balance detected
  // (successful), yields no discovered contract addresses
  const char cusdt_balance_not_detected_response[] = R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":"0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
    })";

  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            cusdt_balance_not_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, false,
                        {});

  // One account, with a balance, yields discovered contract address
  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, false,
                        {"0x6B175474E89094C44Da98b954EedeAC495271d0F"});

  // One account, with a balance, yields no discovered contract addresseses
  // (already in user asset list)
  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, false,
                        {});

  // Reset token list with a fresh token not in user assets
  token_list_json = R"({
    "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2": {
      "name": "Wrapped Eth",
      "logo": "weth.svg",
      "erc20": true,
      "symbol": "WETH",
      "decimals": 18,
      "chainId": "0x1"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // Two accounts, each with the same balance, yields just one discovered
  // contract address
  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_detected_response},
           {"0x2B5AD5c4795c026514f8317c7a215E218DcCD6cF",
            eth_balance_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
                         "0x2B5AD5c4795c026514f8317c7a215E218DcCD6cF"},
                        false, {"0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"});

  // Single account on multiple chains discovers two assets
  asset_discovery_manager_->SetSupportedChainsForTesting(
      {mojom::kMainnetChainId, mojom::kPolygonMainnetChainId});
  token_list_json = R"({
      "0x1111111111111111111111111111111111111111": {
        "name": "1111",
        "logo": "111.svg",
        "erc20": true,
        "symbol": "111",
        "decimals": 18,
        "chainId": "0x1"
      },
      "0x2222222222222222222222222222222222222222": {
        "name": "22222222222",
        "logo": "2222.svg",
        "erc20": true,
        "symbol": "2222",
        "decimals": 18,
        "chainId": "0x89"
      }
     })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_detected_response},
       }},
      {GetNetwork(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, false,
                        {"0x1111111111111111111111111111111111111111",
                         "0x2222222222222222222222222222222222222222"});

  // Multiple accounts with different balances, yields multiple discovered
  // contract addresses Reset token list with a fresh token not in user assets
  token_list_json = R"({
      "0x3333333333333333333333333333333333333333": {
        "name": "3333",
        "logo": "333.svg",
        "erc20": true,
        "symbol": "333",
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
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_not_detected_response},
       }},
      {GetNetwork(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            eth_balance_not_detected_response},
           {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
            eth_balance_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets({"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
                         "0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"},
                        false,
                        {"0x3333333333333333333333333333333333333333",
                         "0x4444444444444444444444444444444444444444"});
}

TEST_F(AssetDiscoveryManagerUnitTest, GetAssetDiscoverySupportedEthChains) {
  // Bypass SetSupportedChainsForTesting by setting to empty list
  asset_discovery_manager_->SetSupportedChainsForTesting({});
  // GetAssetDiscoverySupportedEthChains should return a list of the same size
  // every time
  const std::vector<std::string> chains1 =
      asset_discovery_manager_->GetAssetDiscoverySupportedEthChains();
  const std::vector<std::string> chains2 =
      asset_discovery_manager_->GetAssetDiscoverySupportedEthChains();
  const std::vector<std::string> chains3 =
      asset_discovery_manager_->GetAssetDiscoverySupportedEthChains();
  EXPECT_GT(chains1.size(), 0u);
  EXPECT_EQ(chains1.size(), chains2.size());
  EXPECT_EQ(chains2.size(), chains3.size());
}

}  // namespace brave_wallet
