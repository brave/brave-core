/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include "base/base64.h"
#include "base/json/json_reader.h"
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
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
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
    : public brave_wallet::BraveWalletServiceObserverBase {
 public:
  TestBraveWalletServiceObserverForAssetDiscovery() = default;

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
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(profile_.get()), keyring_service_,
        json_rpc_service_, tx_service, GetPrefs(), GetLocalState());
    asset_discovery_manager_ = std::make_unique<AssetDiscoveryManager>(
        std::make_unique<api_request_helper::APIRequestHelper>(
            net::DefineNetworkTrafficAnnotation("asset_discovery_manager", ""),
            shared_url_loader_factory_),
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

  void SetInterceptors(std::map<GURL, std::string> responses) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, responses](const network::ResourceRequest& request) {
          // If the request url is in responses, add that response
          auto it = responses.find(request.url);
          if (it != responses.end()) {
            // Get the response string
            std::string response = it->second;
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  void SetHTTPRequestTimeoutInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
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
              base::StringPiece request_string;
              if (request.request_body) {
                request_string = request.request_body->elements()
                                     ->at(0)
                                     .As<network::DataElementBytes>()
                                     .AsStringPiece();
              }
              std::string response;
              for (auto const& [address, potential_response] :
                   address_response_map) {
                // Trim leading "0x" from address before searching for it in the
                // it in the request string since it's not included in the
                // calldata
                if ((request_string.find(address.substr(2)) !=
                     std::string::npos) &&  // and request is not a GET
                    !request_string.empty()) {
                  response = potential_response;
                  break;
                }
                // If the request string is empty and there's only one entry
                // in the address_response_map, return that response.
                // This allows us to match GET requests to SimpleHash (which do
                // not have a request body) to the correct response in addition
                // to POST requests to JSON RPC API (which do have a request
                // body and the address is in it).
                if (request_string.empty() &&
                    address_response_map.size() == 1) {
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

  void TestDiscoverAssetsOnAllSupportedChainsAccountsAdded(
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
      const std::map<mojom::CoinType, std::vector<std::string>>& addresses,
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

  void TestFetchNFTsFromSimpleHash(
      const std::string& account_address,
      const std::vector<std::string>& chain_ids,
      mojom::CoinType coin,
      const std::vector<mojom::BlockchainTokenPtr>& expected_nfts) {
    base::RunLoop run_loop;
    asset_discovery_manager_->FetchNFTsFromSimpleHash(
        account_address, chain_ids, coin,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> nfts) {
              ASSERT_EQ(nfts.size(), expected_nfts.size());
              EXPECT_EQ(nfts, expected_nfts);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestDiscoverNFTsOnAllSupportedChains(
      const std::map<mojom::CoinType, std::vector<std::string>>& addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    asset_discovery_manager_->remaining_buckets_ = 1;
    asset_discovery_manager_->DiscoverNFTsOnAllSupportedChains(addresses,
                                                               false);

    // Wait for the the wallet service event to be emitted (meaning asset
    // discovery has totally completed)
    wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
        expected_token_contract_addresses);
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
       DiscoverAssetsOnAllSupportedChainsAccountsAdded) {
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  wallet_service_->SetNftDiscoveryEnabled(true);

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
  TestDiscoverAssetsOnAllSupportedChainsAccountsAdded(
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
  TestDiscoverAssetsOnAllSupportedChainsAccountsAdded(
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
  wallet_service_->SetNftDiscoveryEnabled(true);

  // Verify that in a single call, we can discover assets on multiple Ethereum
  // chains as well as Solana, and one NFT from SimpleHash. Rate limit pref is
  // updated.
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
      {GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
            "owners?chains=ethereum%2Cpolygon&wallet_addresses="
            "0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"),
       {
           {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", R"({
                  "next": null,
                  "nfts": [
                    {
                      "chain": "ethereum",
                      "contract_address": "0x4E1f41613c9084FdB9E34E11fAE9412427480e56",
                      "token_id": "8635",
                      "name": "Level 14 at {24, 19}",
                      "description": "Terraforms by Mathcastles. Onchain land art from a dynamically generated, onchain 3D world.",
                      "image_url": "https://cdn.simplehash.com/assets/69a8608ff3000e44037b58773e6cc62e494bbd7999ae25b60218d92461f54765.svg",
                      "contract": {
                        "type": "ERC721",
                        "name": "Terraforms",
                        "symbol": "TERRAFORMS"
                      },
                      "collection": {
                        "name": "Terraforms by Mathcastles",
                        "description": "Onchain land art from a dynamically generated onchain 3D world.",
                        "image_url": "https://lh3.googleusercontent.com/JYpFUw47L8R8iGOj0uVzPEUlB11A0YNuS3FWwD349ngn6da-PbsrzV6zSqmkNtsfynm0Dpc-rUIr5z9CwsSQq5C0aVenH71OeA",
                        "spam_score": 0
                      }
                    }
                  ]
              })"},
       }},
      {
          GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
               "owners?chains=ethereum%2Cpolygon&wallet_addresses="
               "0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"),
          {
              {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC", R"({
                  "next": null,
                  "previous": null,
                  "nfts": []
              })"},
          },
      },
      {
          GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
               "owners?chains=solana&wallet_addresses="
               "4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"),
          {
              {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF", R"({
                  "next": null,
                  "previous": null,
                  "nfts": []
              })"},
          },
      },
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
       "0x4444444444444444444444444444444444444444",
       "0x4E1f41613c9084FdB9E34E11fAE9412427480e56"});

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
      {
          GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
               "owners?chains=ethereum%2Cpolygon&wallet_addresses="
               "0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"),
          {
              {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
               R"({
            "next": null,
            "previous": null,
            "nfts": [ ]
          })"},
          },
      },
      {
          GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
               "owners?chains=ethereum%2Cpolygon&wallet_addresses="
               "0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"),
          {
              {"0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC",
               R"({
            "next": null,
            "previous": null,
            "nfts": [ ]
          })"},
          },
      },
      {
          GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
               "owners?chains=solana&wallet_addresses="
               "4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"),
          {
              {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF",
               R"({
            "next": null,
            "previous": null,
            "nfts": [ ]
          })"},
          },
      }};
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

TEST_F(AssetDiscoveryManagerUnitTest, GetSimpleHashNftsByWalletUrl) {
  // Empty address yields empty URL
  EXPECT_EQ(asset_discovery_manager_->GetSimpleHashNftsByWalletUrl(
                "", {mojom::kMainnetChainId}),
            GURL(""));

  // Empty chains yields empty URL
  EXPECT_EQ(asset_discovery_manager_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000", {}),
            GURL());

  // One valid chain yields correct URL
  EXPECT_EQ(asset_discovery_manager_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                {mojom::kMainnetChainId}),
            GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
                 "owners?chains=ethereum&wallet_addresses="
                 "0x0000000000000000000000000000000000000000"));

  // Two valid chains yields correct URL
  EXPECT_EQ(asset_discovery_manager_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                {mojom::kMainnetChainId, mojom::kOptimismMainnetChainId}),
            GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
                 "owners?chains=ethereum%2Coptimism&wallet_addresses="
                 "0x0000000000000000000000000000000000000000"));

  // One invalid chain yields empty URL
  EXPECT_EQ(asset_discovery_manager_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                {"chain ID not supported by SimpleHash"}),
            GURL());
}

TEST_F(AssetDiscoveryManagerUnitTest, ParseNFTsFromSimpleHash) {
  std::string json;
  absl::optional<base::Value> json_value;

  // Non dictionary JSON response yields nullopt
  json = R"([])";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  auto result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_FALSE(result);

  // Missing 'nfts' key yields nullopt
  json = R"({"foo": "bar"})";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_FALSE(result);

  // Dictionary type 'nfts' key yields nullopt
  json = R"({"nfts": {}})";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_FALSE(result);

  // Invalid next URL (wrong host) yields empty next URL
  json = R"({
    "next": "https://foo.com/api/v0/nfts/owners?chains=ethereum&wallet_addresses=0x00",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->first, GURL());

  // Invalid next URL (not https) yields empty next URL
  json = R"({
    "next": "http://api.simplehash.com/api/v0/nfts/owners?chains=ethereum&wallet_addresses=0x00",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->first, GURL());

  // Unsupported CoinType yields nullopt (valid otherwise)
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::FIL);

  // Valid, 1 ETH NFT
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_TRUE(result);
  EXPECT_FALSE(result->first.is_valid());
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "0x1111111111111111111111111111111111111111");
  EXPECT_EQ(result->second[0]->name, "Token #1");
  EXPECT_EQ(result->second[0]->logo,
            "https://nftimages-cdn.simplehash.com/1.png");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, true);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "ONE");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "0x1");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kPolygonMainnetChainId);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::ETH);

  // Valid, 2 ETH NFTs
  json = R"({
    "next": "https://api.simplehash.com/api/v0/nfts/next",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x2222222222222222222222222222222222222222",
        "token_id": "2",
        "name": "Token #2",
        "image_url": "https://nftimages-cdn.simplehash.com/2.png",
        "contract": {
          "type": "ERC721"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->first.spec(),
            "https://simplehash.wallet.brave.com/api/v0/nfts/next");
  EXPECT_EQ(result->second.size(), 2u);
  EXPECT_EQ(result->second[0]->contract_address,
            "0x1111111111111111111111111111111111111111");
  EXPECT_EQ(result->second[0]->name, "Token #1");
  EXPECT_EQ(result->second[0]->logo,
            "https://nftimages-cdn.simplehash.com/1.png");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, true);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "ONE");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "0x1");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kPolygonMainnetChainId);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::ETH);

  EXPECT_EQ(result->second[1]->contract_address,
            "0x2222222222222222222222222222222222222222");
  EXPECT_EQ(result->second[1]->name, "Token #2");
  EXPECT_EQ(result->second[1]->logo,
            "https://nftimages-cdn.simplehash.com/2.png");
  EXPECT_EQ(result->second[1]->is_erc20, false);
  EXPECT_EQ(result->second[1]->is_erc721, true);
  EXPECT_EQ(result->second[1]->is_erc1155, false);
  EXPECT_EQ(result->second[1]->is_nft, true);
  // If symbol is null, it should be saved as an empty string
  EXPECT_EQ(result->second[1]->symbol, "");
  EXPECT_EQ(result->second[1]->decimals, 0);
  EXPECT_EQ(result->second[1]->visible, true);
  EXPECT_EQ(result->second[1]->token_id, "0x2");
  EXPECT_EQ(result->second[1]->chain_id, mojom::kMainnetChainId);
  EXPECT_EQ(result->second[1]->coin, mojom::CoinType::ETH);

  // 6 ETH nfts, but only 1 has all necessary keys yields 1 NFT
  //
  // 1. Missing nothing (valid)
  // 2. Missing chain_id
  // 3. Missing contract_address
  // 4. Missing token_id
  // 5. Missing standard
  // 6. Missing spam_score
  json = R"({
    "next": "https://api.simplehash.com/api/v0/nfts/next",
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
          "spam_score": 0
        }
      },
      {
        "contract_address": "0x2222222222222222222222222222222222222222",
        "token_id": "2",
        "contract": {
          "type": "ERC721",
          "symbol": "TWO"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "token_id": "3",
        "contract": {
          "type": "ERC721",
          "symbol": "THREE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x4444444444444444444444444444444444444444",
        "contract": {
          "type": "ERC721",
          "symbol": "FOUR"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x5555555555555555555555555555555555555555",
        "token_id": "5",
        "contract": {
          "symbol": "FIVE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "polygon",
        "contract_address": "0x6666666666666666666666666666666666666666",
        "token_id": "6",
        "contract": {
          "type": "ERC721",
          "symbol": "SIX"
        },
        "collection": {
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);

  // 1 SOL NFT
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "solana",
        "contract_address": "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE",
        "token_id": null,
        "name": "y00t #2623",
        "description": "y00ts is a generative art project of 15,000 NFTs. y00topia is a curated community of builders and creators. Each y00t was designed by De Labs in Los Angeles, CA.",
        "image_url": "https://cdn.simplehash.com/assets/dc78fa011ba46fa12748f1a20ad5e98e1e0b6746dcbfcf409c091dd48d09aee1.png",
        "status": "minted",
        "contract": {
          "type": "NonFungible",
          "name": "y00t #2623",
          "symbol": "Y00T"
        },
        "collection": {
          "spam_score": 0
        },
        "extra_metadata": {
          "is_mutable": true
        }
      }
    ]
  })";

  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE");
  EXPECT_EQ(result->second[0]->name, "y00t #2623");
  EXPECT_EQ(
      result->second[0]->logo,
      "https://cdn.simplehash.com/assets/"
      "dc78fa011ba46fa12748f1a20ad5e98e1e0b6746dcbfcf409c091dd48d09aee1.png");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, false);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "Y00T");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "");
  EXPECT_EQ(result->second[0]->coingecko_id, "");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kSolanaMainnet);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::SOL);

  // An NFT with a spam_score > 0 will be skipped
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "solana",
        "contract_address": "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE",
        "token_id": null,
        "name": "y00t #2623",
        "description": "y00ts is a generative art project of 15,000 NFTs. y00topia is a curated community of builders and creators. Each y00t was designed by De Labs in Los Angeles, CA.",
        "image_url": "https://cdn.simplehash.com/assets/dc78fa011ba46fa12748f1a20ad5e98e1e0b6746dcbfcf409c091dd48d09aee1.png",
        "status": "minted",
        "contract": {
          "type": "NonFungible",
          "name": "y00t #2623",
          "symbol": "Y00T"
        },
        "collection": {
          "spam_score": 100
        },
        "extra_metadata": {
          "is_mutable": true
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = asset_discovery_manager_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 0u);
}

TEST_F(AssetDiscoveryManagerUnitTest, FetchNFTsFromSimpleHash) {
  std::vector<mojom::BlockchainTokenPtr> expected_nfts;
  std::string json;
  std::string json2;
  std::map<GURL, std::string> responses;
  GURL url;

  // Empty account address yields empty expected_nfts
  TestFetchNFTsFromSimpleHash("", {mojom::kMainnetChainId},
                              mojom::CoinType::ETH, expected_nfts);

  // Empty chain IDs yields empty expected_nfts
  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000", {},
                              mojom::CoinType::ETH, expected_nfts);

  // Unsupported chain ID yields empty expected_nfts
  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000", {},
                              mojom::CoinType::FIL, expected_nfts);

  // Non 2xx response yields empty expected_nfts
  SetHTTPRequestTimeoutInterceptor();
  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                              {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                              expected_nfts);

  // 1 NFT is parsed
  json = R"({
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
          "spam_score": 0
        }
      }
    ]
  })";
  auto nft1 = mojom::BlockchainToken::New();
  nft1->chain_id = mojom::kPolygonMainnetChainId;
  nft1->contract_address = "0x1111111111111111111111111111111111111111";
  nft1->token_id = "0x1";
  nft1->is_erc721 = true;
  nft1->is_erc1155 = false;
  nft1->is_erc20 = false;
  nft1->is_nft = true;
  nft1->symbol = "ONE";
  nft1->coin = mojom::CoinType::ETH;
  expected_nfts.push_back(std::move(nft1));
  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum%2Coptimism&wallet_addresses="
      "0x0000000000000000000000000000000000000000");
  responses[url] = json;
  SetInterceptors(responses);
  TestFetchNFTsFromSimpleHash(
      "0x0000000000000000000000000000000000000000",
      {mojom::kMainnetChainId, mojom::kOptimismMainnetChainId},
      mojom::CoinType::ETH, expected_nfts);

  // If 'next' page url is present, it should make another request
  responses.clear();
  json = R"({
    "next": "https://api.simplehash.com/api/v0/nfts/next",
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
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json;
  GURL next_url = GURL("https://simplehash.wallet.brave.com/api/v0/nfts/next");
  json2 = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "nft_id": "ethereum.0x5555555555555555555555555555555555555555.555555555555",
        "chain": "ethereum",
        "contract_address": "0x5555555555555555555555555555555555555555",
        "token_id": "555555555555",
        "contract": {
          "type": "ERC721",
          "symbol": "FIVE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  responses[next_url] = json2;
  SetInterceptors(responses);
  auto nft2 = mojom::BlockchainToken::New();
  nft2->chain_id = mojom::kMainnetChainId;
  nft2->contract_address = "0x5555555555555555555555555555555555555555";
  nft2->token_id = "0x8159b108e3";  // "555555555555"
  nft2->is_erc20 = false;
  nft2->is_erc721 = true;
  nft2->is_erc1155 = false;
  nft2->is_nft = true;
  nft2->symbol = "FIVE";
  nft2->coin = mojom::CoinType::ETH;
  expected_nfts.push_back(std::move(nft2));
  TestFetchNFTsFromSimpleHash(
      "0x0000000000000000000000000000000000000000",
      {mojom::kMainnetChainId, mojom::kOptimismMainnetChainId},
      mojom::CoinType::ETH, expected_nfts);
}

TEST_F(AssetDiscoveryManagerUnitTest, DiscoverNFTsOnAllSupportedChains) {
  std::map<mojom::CoinType, std::vector<std::string>> addresses;
  std::vector<std::string> expected_contract_addresses;
  std::map<GURL, std::string> responses;
  std::string json;
  std::string json2;
  GURL url;

  // Empty addresses yields empty expected_contract_addresses
  wallet_service_->SetNftDiscoveryEnabled(true);
  TestDiscoverNFTsOnAllSupportedChains(addresses, expected_contract_addresses);

  // 1 ETH address yields 1 discovered NFT
  asset_discovery_manager_->SetSupportedChainsForTesting(
      {mojom::kMainnetChainId, mojom::kPolygonMainnetChainId});
  addresses[mojom::CoinType::ETH] = {
      "0x0000000000000000000000000000000000000000"};
  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum%2Cpolygon&wallet_addresses="
      "0x0000000000000000000000000000000000000000");
  json = R"({
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
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json;
  SetInterceptors(responses);
  expected_contract_addresses.push_back(
      "0x1111111111111111111111111111111111111111");
  // First test nothing is discovered when NFT discovery is not enabled.
  wallet_service_->SetNftDiscoveryEnabled(false);
  TestDiscoverNFTsOnAllSupportedChains(addresses, {});
  // Enable and verify 1 ETH address yields 1 discovered NFT
  wallet_service_->SetNftDiscoveryEnabled(true);
  TestDiscoverNFTsOnAllSupportedChains(addresses, expected_contract_addresses);

  // 2 ETH addresses (2 requests), yields 4 discovered NFTs (1 from one address,
  // and 3 from the other
  expected_contract_addresses.clear();
  addresses.clear();
  asset_discovery_manager_->SetSupportedChainsForTesting(
      {mojom::kMainnetChainId});
  addresses[mojom::CoinType::ETH].push_back(
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961");
  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum&wallet_addresses="
      "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961");
  json = R"({
    "next": null,
    "nfts": [
      {
        "chain": "ethereum",
        "contract_address": "0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85",
        "token_id": "53762001732575849527995595036249427730510390651723189221519398504820492711584",
        "name": "stochasticparrot.eth",
        "description": "stochasticparrot.eth, an ENS name.",
        "image_url": "https://cdn.simplehash.com/assets/6e174a2e0091ffd5c0c63904366a62da8890508b01e7e85b13d5475b038e6544.svg",
        "last_sale": null,
        "contract": {
          "type": "ERC721",
          "name": null,
          "symbol": "ENS"
        },
        "collection": {
          "name": "ENS: Ethereum Name Service",
          "description": "Ethereum Name Service (ENS) domains are secure domain names for the decentralized world. ENS domains provide a way for users to map human readable names to blockchain and non-blockchain resources, like Ethereum addresses, IPFS hashes, or website URLs. ENS domains can be bought and sold on secondary markets.",
          "image_url": "https://lh3.googleusercontent.com/yXNjPUCCTHyvYNarrb81ln31I6hUIaoPzlGU8kki-OohiWuqxfrIkMaOdLzcO4iGuXcvE5mgCZ-ds9tZotEJi3hdkNusheEK_w2V",
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x9251dEC8DF720C2ADF3B6f46d968107cbBADf4d4",
        "token_id": "3176",
        "name": "1337 skulls #3176",
        "description": "1337 skulls is a collection of 7,331 pixel art skulls, deployed fully on-chain with a public domain license.  600+ traits created from new, original art and referencing 30+ existing cc0 NFT projects.  Free mint.  0% royalties.  No roadmap.  Just 1337.",
        "image_url": "https://cdn.simplehash.com/assets/67cd1f24395a09ccfc0693d231671738ab8d1976a4a46f5ba6f091076ee942c9.svg",
        "contract": {
          "type": "ERC721",
          "name": "1337 skulls",
          "symbol": "1337skulls"
        },
        "collection": {
          "name": "1337 skulls",
          "description": "1337 skulls is a collection of 7,331 pixel art skulls, deployed fully on-chain with a public domain license.  600+ traits created from new, original art and referencing 30+ existing cc0 NFT projects.  Free mint.  0% royalties.  No roadmap.  Just 1337.",
          "image_url": "https://lh3.googleusercontent.com/8vMgdfdfIkn_c9iVSAmWJ0S3cQDSWSgYUU2hYC4sUBHow5wJIgoRjGPREnQwjE5kdyu0e6UNQ5NXING82kIubdU4p5j8XpT47rQ",
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x4b10701Bfd7BFEdc47d50562b76b436fbB5BdB3B",
        "token_id": "5929",
        "name": "Lil Noun 5929",
        "description": "Lil Noun 5929 is a member of the Lil Nouns DAO",
        "image_url": "https://cdn.simplehash.com/assets/8c3a6098e6387c9f129a45adf79ceaa32a4c52a5aaf4cc21d29289fd98000b07.svg",
        "contract": {
          "type": "ERC721",
          "name": "LilNoun",
          "symbol": "LILNOUN"
        },
        "collection": {
          "name": "Lil Nouns",
          "description": "One Lil Noun, every 15 minutes, forever.\r\n\r\nlilnouns.wtf",
          "image_url": "https://lh3.googleusercontent.com/Bd9JbbJl9cmaFCtws9ZgWdsoVYWt_N8XrJ_9s82LTD-chFitIDck8hHt2dpofekr6PvlKwFT-Zh-lOvcJbcFpI2N3YCkKZoQUCk",
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json;
  expected_contract_addresses.push_back(
      "0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85");
  expected_contract_addresses.push_back(
      "0x9251dEC8DF720C2ADF3B6f46d968107cbBADf4d4");
  expected_contract_addresses.push_back(
      "0x4b10701Bfd7BFEdc47d50562b76b436fbB5BdB3B");

  addresses[mojom::CoinType::ETH].push_back(
      "0x16e4476c8fDDc552e3b1C4b8b56261d85977fE52");
  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum&wallet_addresses="
      "0x16e4476c8fDDc552e3b1C4b8b56261d85977fE52");
  json2 = R"({
    "next": null,
    "nfts": [
      {
        "chain": "ethereum",
        "contract_address": "0x4E1f41613c9084FdB9E34E11fAE9412427480e56",
        "token_id": "8635",
        "name": "Level 14 at {24, 19}",
        "description": "Terraforms by Mathcastles. Onchain land art from a dynamically generated, onchain 3D world.",
        "image_url": "https://cdn.simplehash.com/assets/69a8608ff3000e44037b58773e6cc62e494bbd7999ae25b60218d92461f54765.svg",
        "contract": {
          "type": "ERC721",
          "name": "Terraforms",
          "symbol": "TERRAFORMS"
        },
        "collection": {
          "name": "Terraforms by Mathcastles",
          "description": "Onchain land art from a dynamically generated onchain 3D world.",
          "image_url": "https://lh3.googleusercontent.com/JYpFUw47L8R8iGOj0uVzPEUlB11A0YNuS3FWwD349ngn6da-PbsrzV6zSqmkNtsfynm0Dpc-rUIr5z9CwsSQq5C0aVenH71OeA",
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json2;
  expected_contract_addresses.push_back(
      "0x4E1f41613c9084FdB9E34E11fAE9412427480e56");

  SetInterceptors(responses);
  TestDiscoverNFTsOnAllSupportedChains(addresses, expected_contract_addresses);

  // Making the same request again should not yield any new discovered NFTs
  // since they are have already been discovered and added
  TestDiscoverNFTsOnAllSupportedChains(addresses, {});
}

}  // namespace brave_wallet
