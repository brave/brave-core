/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_task.h"

#include <optional>
#include <string_view>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/simple_hash_client.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
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
constexpr char kJsonRpcResponseTemplate[] = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"$1"
  })";

std::string formatJsonRpcResponse(const std::string& value) {
  return base::ReplaceStringPlaceholders(kJsonRpcResponseTemplate, {value},
                                         nullptr);
}

constexpr char kEthBalanceDetectedResult[] =
    "0x"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "0000000000000000000000000000000000000000000000000000000000000001"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "0000000000000000000000000000000000000000000000000000000000000001"
    "0000000000000000000000000000000000000000000000000000000000000040"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "000000000000000000000000000000000000000000000006e83695ab1f893c00";

constexpr char kEthBalanceNotDetectedResult[] =
    "0x"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "0000000000000000000000000000000000000000000000000000000000000001"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "0000000000000000000000000000000000000000000000000000000000000001"
    "0000000000000000000000000000000000000000000000000000000000000040"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "0000000000000000000000000000000000000000000000000000000000000000";

constexpr char kEthErrorFetchingBalanceResult[] =
    "0x"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "0000000000000000000000000000000000000000000000000000000000000001"
    "0000000000000000000000000000000000000000000000000000000000000020"
    "0000000000000000000000000000000000000000000000000000000000000000"
    "0000000000000000000000000000000000000000000000000000000000000040"
    "0000000000000000000000000000000000000000000000000000000000000000";

}  // namespace

class TestBraveWalletServiceObserverForAssetDiscoveryTask
    : public brave_wallet::BraveWalletServiceObserverBase {
 public:
  TestBraveWalletServiceObserverForAssetDiscoveryTask() = default;

  void OnDiscoverAssetsStarted() override {
    on_discover_assets_started_fired_ = true;
  }

  void OnDiscoverAssetsCompleted(
      std::vector<mojom::BlockchainTokenPtr> discovered_assets) override {
    ASSERT_EQ(expected_contract_addresses_.size(), discovered_assets.size());
    for (size_t i = 0; i < discovered_assets.size(); i++) {
      EXPECT_EQ(expected_contract_addresses_[i],
                discovered_assets[i]->contract_address);
    }
    on_discover_assets_completed_fired_ = true;
    if (run_loop_asset_discovery_) {
      run_loop_asset_discovery_->Quit();
    }
  }

  void WaitForOnDiscoverAssetsCompleted(
      const std::vector<std::string>& addresses) {
    expected_contract_addresses_ = addresses;
    run_loop_asset_discovery_ = std::make_unique<base::RunLoop>();
    run_loop_asset_discovery_->Run();
  }

  bool OnDiscoverAssetsStartedFired() {
    return on_discover_assets_started_fired_;
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
    on_discover_assets_started_fired_ = false;
    on_discover_assets_completed_fired_ = false;
  }

 private:
  std::unique_ptr<base::RunLoop> run_loop_asset_discovery_;
  std::vector<std::string> expected_contract_addresses_;
  bool on_discover_assets_started_fired_ = false;
  bool on_discover_assets_completed_fired_ = false;
  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      observer_receiver_{this};
};

class AssetDiscoveryTaskUnitTest : public testing::Test {
 public:
  AssetDiscoveryTaskUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)),
        task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~AssetDiscoveryTaskUnitTest() override = default;

 protected:
  void SetUp() override {
    scoped_feature_list_.InitWithFeatures(
        {features::kNativeBraveWalletFeature,
         features::kBraveWalletAnkrBalancesFeature},
        {});

    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(profile_.get()), GetPrefs(),
        GetLocalState());
    json_rpc_service_ = wallet_service_->json_rpc_service();
    keyring_service_ = wallet_service_->keyring_service();
    tx_service_ = wallet_service_->tx_service();

    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            net::DefineNetworkTrafficAnnotation("asset_discovery_manager", ""),
            shared_url_loader_factory_);
    simple_hash_client_ =
        std::make_unique<SimpleHashClient>(shared_url_loader_factory_);
    asset_discovery_task_ = std::make_unique<AssetDiscoveryTask>(
        *api_request_helper_, *simple_hash_client_, *wallet_service_,
        *json_rpc_service_, GetPrefs());
    wallet_service_observer_ =
        std::make_unique<TestBraveWalletServiceObserverForAssetDiscoveryTask>();
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

  // SetInterceptorForDiscoverAnkrOrSolAssets takes a map of addresses to
  // responses and adds the response if the address if found in the request
  // string
  void SetInterceptorForDiscoverAnkrOrSolAssets(
      const GURL& intended_url,
      const std::map<std::string, std::string>& requests) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, intended_url, requests](const network::ResourceRequest& request) {
          if (request.url.spec() == intended_url) {
            std::string_view request_string(request.request_body->elements()
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
              std::string_view request_string;
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

  void TestDiscoverAnkrAssets(
      const std::vector<std::string>& chain_ids,
      const std::vector<std::string>& account_addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    base::RunLoop run_loop;
    asset_discovery_task_->DiscoverAnkrTokens(
        chain_ids, account_addresses,
        base::BindLambdaForTesting(
            [&](const std::vector<mojom::BlockchainTokenPtr>
                    discovered_assets) {
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(discovered_assets[i]->contract_address,
                          expected_token_contract_addresses[i]);
              }
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestDiscoverEthAssets(
      const std::vector<std::string>& chain_ids,
      const std::vector<std::string>& account_addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    base::RunLoop run_loop;
    asset_discovery_task_->DiscoverERC20sFromRegistry(
        chain_ids, account_addresses,
        base::BindLambdaForTesting(
            [&](const std::vector<mojom::BlockchainTokenPtr>
                    discovered_assets) {
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(discovered_assets[i]->contract_address,
                          expected_token_contract_addresses[i]);
              }
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestDiscoverSolAssets(
      const std::vector<std::string>& account_addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    base::RunLoop run_loop;
    asset_discovery_task_->DiscoverSPLTokensFromRegistry(
        account_addresses,
        base::BindLambdaForTesting(
            [&](const std::vector<mojom::BlockchainTokenPtr>
                    discovered_assets) {
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(discovered_assets[i]->contract_address,
                          expected_token_contract_addresses[i]);
              }
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestDiscoverNFTsOnAllSupportedChains(
      const std::map<mojom::CoinType, std::vector<std::string>>& chain_ids,
      const std::map<mojom::CoinType, std::vector<std::string>>& addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    base::RunLoop run_loop;
    asset_discovery_task_->DiscoverNFTs(
        chain_ids, addresses,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> discovered_assets) {
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(discovered_assets[i]->contract_address,
                          expected_token_contract_addresses[i]);
              }
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestDiscoverAssets(
      const std::map<mojom::CoinType, std::vector<std::string>>&
          account_addresses,
      const std::vector<std::string>& expected_token_contract_addresses) {
    base::RunLoop run_loop;
    asset_discovery_task_->DiscoverAssets(
        {}, {}, account_addresses, base::BindLambdaForTesting([&]() {
          wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
              expected_token_contract_addresses);
          EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsStartedFired());
          EXPECT_TRUE(
              wallet_service_observer_->OnDiscoverAssetsCompletedFired());
          run_loop.Quit();
        }));
    run_loop.Run();
    wallet_service_observer_->Reset();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return wallet_service_->network_manager()->GetNetworkURL(chain_id, coin);
  }

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<TestBraveWalletServiceObserverForAssetDiscoveryTask>
      wallet_service_observer_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveWalletService> wallet_service_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  std::unique_ptr<SimpleHashClient> simple_hash_client_;
  std::unique_ptr<AssetDiscoveryTask> asset_discovery_task_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<TxService> tx_service_;
  base::test::ScopedFeatureList scoped_feature_list_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(AssetDiscoveryTaskUnitTest, DiscoverAnkrTokens) {
  // Empty chain ids and account addresses
  TestDiscoverAnkrAssets({}, {}, {});

  // Empty chain ids
  TestDiscoverAnkrAssets({}, {"0xa92d461a9a988a7f11ec285d39783a637fdd6ba4"},
                         {});

  // Empty account addresses
  TestDiscoverAnkrAssets({mojom::kMainnetChainId}, {}, {});

  std::map<std::string, std::string> requests = {
      {"0xa92d461a9a988a7f11ec285d39783a637fdd6ba4", R"(
        {
          "jsonrpc": "2.0",
          "id": 1,
          "result": {
            "totalBalanceUsd": "4915134435857.581297310767673907",
            "assets": [
              {
                "blockchain": "polygon",
                "tokenName": "USD Coin",
                "tokenSymbol": "USDC",
                "tokenDecimals": "6",
                "tokenType": "ERC20",
                "contractAddress": "0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
                "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
                "balance": "8.202765",
                "balanceRawInteger": "8202765",
                "balanceUsd": "8.202765",
                "tokenPrice": "1",
                "thumbnail": "usdc.png"
              }
            ]
          }
        })"},
      {"0xdac17f958d2ee523a2206206994597c13d831ec7", R"(
        {
          "jsonrpc": "2.0",
          "id": 1,
          "result": {
            "totalBalanceUsd": "4915134435857.581297310767673907",
            "assets": [
              {
                "blockchain": "eth",
                "tokenName": "Dai Stablecoin",
                "tokenSymbol": "DAI",
                "tokenDecimals": 18,
                "tokenType": "ERC20",
                "contractAddress": "0x6b175474e89094c44da98b954eedeac495271d0f",
                "holderAddress": "0xdac17f958d2ee523a2206206994597c13d831ec7",
                "balance": "21.645537148041723435",
                "balanceRawInteger": "21645537148041723435",
                "balanceUsd": "21.64134170578332378",
                "tokenPrice": "0.999806175183840184",
                "thumbnail": "dai.png"
              }
            ]
          }
        })"},
  };

  SetInterceptorForDiscoverAnkrOrSolAssets(GURL(kAnkrAdvancedAPIBaseURL),
                                           requests);
  TestDiscoverAnkrAssets(
      {mojom::kPolygonMainnetChainId, mojom::kMainnetChainId},
      {"0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
       "0xdac17f958d2ee523a2206206994597c13d831ec7"},
      {"0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
       "0x6b175474e89094c44da98b954eedeac495271d0f"});
}

TEST_F(AssetDiscoveryTaskUnitTest, DiscoverERC20sFromRegistry) {
  std::vector<std::string> chain_ids;
  chain_ids.push_back(mojom::kMainnetChainId);
  const std::string eth_balance_detected_response =
      formatJsonRpcResponse(kEthBalanceDetectedResult);
  const std::string eth_balance_not_detected_response =
      formatJsonRpcResponse(kEthBalanceNotDetectedResult);
  const std::string eth_error_fetching_balance_response =
      formatJsonRpcResponse(kEthErrorFetchingBalanceResult);

  TestDiscoverEthAssets({}, {}, {});

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
  std::map<GURL, std::map<std::string, std::string>> requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_not_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets(chain_ids,
                        {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {});

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
  TestDiscoverEthAssets(chain_ids,
                        {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {});

  // One account returns the cUSDT token response for no balance detected
  // (successful), yields no discovered contract addresses
  const char cusdt_balance_not_detected_result[] =
      "0x"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000000000000000000000000000000000000000060"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000";
  const std::string cusdt_balance_not_detected_response =
      formatJsonRpcResponse(cusdt_balance_not_detected_result);

  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            cusdt_balance_not_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets(chain_ids,
                        {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {});

  // One account, with a balance, yields discovered contract address
  requests = {
      {GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH),
       {
           {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
            eth_balance_detected_response},
       }},
  };
  SetInterceptorForDiscoverEthAssets(requests);
  TestDiscoverEthAssets(chain_ids,
                        {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
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
  TestDiscoverEthAssets(chain_ids,
                        {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {});

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
  TestDiscoverEthAssets(chain_ids,
                        {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
                         "0x2B5AD5c4795c026514f8317c7a215E218DcCD6cF"},
                        {"0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"});

  // Single account on multiple chains discovers two assets
  chain_ids.clear();
  chain_ids.push_back(mojom::kMainnetChainId);
  chain_ids.push_back(mojom::kPolygonMainnetChainId);
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
  TestDiscoverEthAssets(chain_ids,
                        {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
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
  TestDiscoverEthAssets(chain_ids,
                        {"0xBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
                         "0xCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"},
                        {"0x3333333333333333333333333333333333333333",
                         "0x4444444444444444444444444444444444444444"});
}

TEST_F(AssetDiscoveryTaskUnitTest, DecodeMintAddress) {
  // Invalid (data too short)
  std::optional<std::vector<uint8_t>> data_short = base::Base64Decode("YQ==");
  ASSERT_TRUE(data_short);
  std::optional<SolanaAddress> mint_address =
      asset_discovery_task_->DecodeMintAddress(*data_short);
  ASSERT_FALSE(mint_address);

  // Valid
  std::optional<std::vector<uint8_t>> data = base::Base64Decode(
      "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+"
      "lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/"
      "qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAA");
  ASSERT_TRUE(data);
  mint_address = asset_discovery_task_->DecodeMintAddress(*data);
  ASSERT_TRUE(mint_address);
  EXPECT_EQ(mint_address.value().ToBase58(),
            "88j24JNwWLmJCjn2tZQ5jJzyaFtnusS2qsKup9NeDnd8");
}

TEST_F(AssetDiscoveryTaskUnitTest, DiscoverSPLTokensFromRegistry) {
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
  std::string response_template = R"({
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
              $1,
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
              $2,
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
  std::string base64AccountData1 =
      "z6cxAUoRHIupvmezOL4EAsTLlwKTgwxzCg/"
      "xcNWSEu42kEWUG3BArj8SJRSnd1faFt2Tm0Ey/"
      "qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAA";
  std::string base64AccountData2 =
      "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+"
      "lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/"
      "qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAA";

  std::string response = base::ReplaceStringPlaceholders(
      response_template, {base64AccountData1, base64AccountData2}, nullptr);
  SetInterceptor(expected_network_url, response);
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
  response_template = R"({
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
              $1,
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
              $2,
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
  base64AccountData1 =
      "l/"
      "QUsV2gleWOBK7DT7McygX06DWutQjr6AinX510aVU2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/"
      "qtGnPdOOlQluoAsOSueAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "QAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAA";
  base64AccountData2 =
      "iOBUDkpieWsUu53IBhROGzPicXkIYV2OIGUzsFvIlvU2kEWUG3BArj8SJRSnd1faFt2Tm0Ey"
      "/qtGnPdOOlQlugDkC1QCAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
      "AAAA";
  response = base::ReplaceStringPlaceholders(
      response_template, {base64AccountData1, base64AccountData2}, nullptr);

  // Owner 8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB has mints
  // 7vfCXTUXx5WJV5JADk17DUJ4ksgau7utNKj4b963voxs and
  // 4zLh7YPr8NfrNP4bzTXaYaE72QQc3A8mptbtqUspRz5g
  // const std::string second_response = R"({
  response_template = R"({
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
              $1,
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
              $2,
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
  base64AccountData1 =
      "O0NuqIea7HUvvwtwGehJ95pVsBSH6xpS3rvbymg9TMNuN8HO+P8En+NLC+"
      "JfUEsxJnxEYiI50JuYlZKuo/"
      "DnTAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  base64AccountData2 =
      "ZuUYihMIoduQttMfP73KjD3yZ4yBEt/"
      "dPRksWjzEV6huN8HO+P8En+NLC+JfUEsxJnxEYiI50JuYlZKuo/"
      "DnTCeWHwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  const std::string second_response = base::ReplaceStringPlaceholders(
      response_template, {base64AccountData1, base64AccountData2}, nullptr);
  const std::map<std::string, std::string> requests = {
      {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF", response},
      {"8RFACUfst117ARQLezvK4cKVR8ZHvW2xUfdUoqWnTuEB", second_response},
  };
  SetInterceptorForDiscoverAnkrOrSolAssets(expected_network_url, requests);

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

TEST_F(AssetDiscoveryTaskUnitTest, DiscoverNFTs) {
  std::map<mojom::CoinType, std::vector<std::string>> chain_ids;
  std::vector<std::string> eth_chain_ids;
  eth_chain_ids.push_back(mojom::kMainnetChainId);
  eth_chain_ids.push_back(mojom::kPolygonMainnetChainId);
  chain_ids[mojom::CoinType::ETH] = eth_chain_ids;
  std::map<mojom::CoinType, std::vector<std::string>> addresses;
  std::vector<std::string> expected_contract_addresses;
  std::map<GURL, std::string> responses;
  std::string json;
  std::string json2;
  GURL url;

  // Empty addresses yields empty expected_contract_addresses
  wallet_service_->SetNftDiscoveryEnabled(true);
  TestDiscoverNFTsOnAllSupportedChains(chain_ids, addresses,
                                       expected_contract_addresses);

  // 1 ETH address yields 1 discovered NFT
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
  TestDiscoverNFTsOnAllSupportedChains(chain_ids, addresses, {});

  // Enable and verify 1 ETH address yields 1 discovered NFT
  wallet_service_->SetNftDiscoveryEnabled(true);
  TestDiscoverNFTsOnAllSupportedChains(chain_ids, addresses,
                                       expected_contract_addresses);

  // 2 ETH addresses (2 requests), yields 4 discovered NFTs (1 from one address,
  // and 3 from the other
  expected_contract_addresses.clear();
  addresses.clear();
  eth_chain_ids.clear();
  eth_chain_ids.push_back(mojom::kMainnetChainId);
  chain_ids[mojom::CoinType::ETH] = eth_chain_ids;
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
        "token_id": "537620017325758495279955950362494277305103",
        "name": "stochasticparrot.eth",
        "description": "stochasticparrot.eth, an ENS name.",
        "image_url": "https://cdn.simplehash.com/assets/6e174a2e0.svg",
        "last_sale": null,
        "contract": {
          "type": "ERC721",
          "name": null,
          "symbol": "ENS"
        },
        "collection": {
          "name": "ENS: Ethereum Name Service",
          "description": "Ethereum Name Service (ENS) domains.",
          "image_url": "https://lh3.googleusercontent.com/yXNjPUCCTHyvYNarr",
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x9251dEC8DF720C2ADF3B6f46d968107cbBADf4d4",
        "token_id": "3176",
        "name": "1337 skulls #3176",
        "description": "1337 skulls is a collection of pixel art skulls.",
        "image_url": "https://cdn.simplehash.com/assets/67cd1f24395a09ccf.svg",
        "contract": {
          "type": "ERC721",
          "name": "1337 skulls",
          "symbol": "1337skulls"
        },
        "collection": {
          "name": "1337 skulls",
          "description": "1337 skulls is a collection of pixel art skulls.",
          "image_url": "https://lh3.googleusercontent.com/8vMgdfdfIkn_c9iV",
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x4b10701Bfd7BFEdc47d50562b76b436fbB5BdB3B",
        "token_id": "5929",
        "name": "Lil Noun 5929",
        "description": "Lil Noun 5929 is a member of the Lil Nouns DAO",
        "image_url": "https://cdn.simplehash.com/assets/8c3a6098e6387c9f129a.svg",
        "contract": {
          "type": "ERC721",
          "name": "LilNoun",
          "symbol": "LILNOUN"
        },
        "collection": {
          "name": "Lil Nouns",
          "description": "One Lil Noun, every 15 minutes, forever.",
          "image_url": "https://lh3.googleusercontent.com/Bd9JbbJl9cmaFCtws9Zg",
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
        "description": "Terraforms by Mathcastles.",
        "image_url": "https://cdn.simplehash.com/assets/69a8608ff30.svg",
        "contract": {
          "type": "ERC721",
          "name": "Terraforms",
          "symbol": "TERRAFORMS"
        },
        "collection": {
          "name": "Terraforms by Mathcastles",
          "description": "Onchain land art.",
          "image_url": "https://lh3.googleusercontent.com/71OeA",
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json2;
  expected_contract_addresses.push_back(
      "0x4E1f41613c9084FdB9E34E11fAE9412427480e56");

  SetInterceptors(responses);
  TestDiscoverNFTsOnAllSupportedChains(chain_ids, addresses,
                                       expected_contract_addresses);

  // Making the same request again should not yield any new discovered NFTs
  // since they are have already been discovered and added
  TestDiscoverNFTsOnAllSupportedChains(chain_ids, addresses, {});
}

TEST_F(AssetDiscoveryTaskUnitTest, DiscoverAssets) {
  // Verify DiscoverAssetsStarted and DiscoverAssetsCompleted have both fired
  TestDiscoverAssets({}, {});
}

}  // namespace brave_wallet
