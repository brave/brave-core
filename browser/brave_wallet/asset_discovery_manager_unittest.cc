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

const char kMnemonic1[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";
const char kPasswordBrave[] = "brave";

void UpdateCustomNetworks(PrefService* prefs,
                          std::vector<base::Value::Dict>* values) {
  ScopedDictPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value::List* list = update->EnsureList(kEthereumPrefKey);
  list->clear();
  for (auto& it : *values) {
    list->Append(std::move(it));
  }
}

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
      GURL& intended_url,
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

  // SetDiscoverAssetsOnAllSupportedChainsInterceptor sets the response
  // based on the requested URL and verifies the from / to blocks
  // specified in the eth_getLogs query is expected for each URL/chain ID
  // requested.
  void SetDiscoverAssetsOnAllSupportedChainsInterceptor(
      const std::map<std::string, std::string> responses,
      const std::map<std::string, std::string> expected_from_blocks,
      const std::map<std::string, std::string> expected_to_blocks) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, responses, expected_from_blocks,
         expected_to_blocks](const network::ResourceRequest& request) {
          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());

          // Verify fromBlock and toBlock's requested match expected for ETH
          // chains
          auto it = expected_from_blocks.find(request.url.spec());
          if (it != expected_from_blocks.end()) {
            const auto& expected_from_block = it->second;
            EXPECT_NE(request_string.find(base::StringPrintf(
                          "\"fromBlock\":\"%s\"", expected_from_block.c_str())),
                      std::string::npos);
          }
          // Same for toBlock
          it = expected_to_blocks.find(request.url.spec());
          if (it != expected_to_blocks.end()) {
            const auto& expected_to_block = it->second;
            EXPECT_NE(request_string.find(base::StringPrintf(
                          "\"toBlock\":\"%s\"", expected_to_block.c_str())),
                      std::string::npos);
          }
          url_loader_factory_.ClearResponses();
          for (const auto& kv : responses) {
            url_loader_factory_.AddResponse(kv.first, kv.second);
          }
          return;
        }));
  }

  void TestDiscoverSolAssets(
      const std::vector<std::string>& account_addresses,
      const std::vector<std::string>& expected_contract_addresses) {
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::string& chain_id,
                const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
                absl::optional<mojom::ProviderError> error,
                const std::string& error_message) {
              ASSERT_FALSE(error);
              ASSERT_EQ(discovered_assets.size(),
                        expected_contract_addresses.size());
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(discovered_assets[i]->contract_address,
                          expected_contract_addresses[i]);
              }
            }));
    asset_discovery_manager_->DiscoverSolAssets(account_addresses, false);
    base::RunLoop().RunUntilIdle();
  }

  void TestDiscoverAssets(
      const std::string& chain_id,
      mojom::CoinType coin,
      const std::vector<std::string>& account_addresses,
      const std::vector<std::string>& expected_token_contract_addresses,
      mojom::ProviderError expected_error,
      const std::string& expected_error_message,
      const std::string& expected_next_asset_discovery_from_block) {
    // Set remaining chains to 1 in order since this value needs to end at
    // 0 by the end of the DiscoverEthAssets call in order to trigger the
    // event and it will not be set by an outer
    // DiscoverAssetsOnAllSupportedChains* call in this unit test.
    asset_discovery_manager_->remaining_chains_ = 1;
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::string& chain_id,
                const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
                absl::optional<mojom::ProviderError> error,
                const std::string& error_message) {
              EXPECT_EQ(chain_id, chain_id);
              ASSERT_TRUE(error);
              EXPECT_EQ(*error, expected_error);
              EXPECT_EQ(expected_error_message, error_message);
              ASSERT_EQ(expected_token_contract_addresses.size(),
                        discovered_assets.size());
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(expected_token_contract_addresses[i],
                          discovered_assets[i]->contract_address);
              }
            }));
    asset_discovery_manager_->DiscoverEthAssets(
        chain_id, mojom::CoinType::ETH, account_addresses, false,
        kEthereumBlockTagEarliest, kEthereumBlockTagLatest);
    wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
        expected_token_contract_addresses);
    auto next_asset_discovery_from_blocks =
        GetPrefs()->GetDict(kBraveWalletNextAssetDiscoveryFromBlocks).Clone();
    const auto path = base::StrCat({kEthereumPrefKey, ".", chain_id});
    const std::string* next_asset_discovery_from_block =
        next_asset_discovery_from_blocks.FindStringByDottedPath(path);
    if (next_asset_discovery_from_block) {
      EXPECT_EQ(*next_asset_discovery_from_block,
                expected_next_asset_discovery_from_block);
    } else {
      ASSERT_EQ(expected_next_asset_discovery_from_block, "");
    }
    wallet_service_observer_->Reset();
  }

  void TestDiscoverAssetsOnAllSupportedChainsAccountsAdded(
      mojom::CoinType coin,
      const std::vector<std::string>& account_addresses,
      const base::Time expected_assets_last_discovered_at_pref,
      const base::Value::Dict& expected_next_asset_discovery_from_blocks,
      const std::vector<std::string>& expected_token_contract_addresses = {}) {
    std::vector<std::string> expected_chain_ids_remaining;
    if (coin == mojom::CoinType::ETH) {
      expected_chain_ids_remaining =
          GetAssetDiscoverySupportedEthChainsForTest();
    } else {
      expected_chain_ids_remaining = {mojom::kSolanaMainnet};
    }
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::string& chain_id,
                const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
                absl::optional<mojom::ProviderError> error,
                const std::string& error_message) {
              expected_chain_ids_remaining.erase(
                  std::remove(expected_chain_ids_remaining.begin(),
                              expected_chain_ids_remaining.end(), chain_id),
                  expected_chain_ids_remaining.end());
            }));
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChainsAccountsAdded(
        coin, account_addresses);
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(expected_chain_ids_remaining.size(), 0u);
    EXPECT_EQ(GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt),
              expected_assets_last_discovered_at_pref);
    EXPECT_EQ(GetPrefs()->GetDict(kBraveWalletNextAssetDiscoveryFromBlocks),
              expected_next_asset_discovery_from_blocks);
    EXPECT_FALSE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
    wallet_service_observer_->Reset();
  }

  void TestDiscoverAssetsOnAllSupportedChainsRefresh(
      std::map<mojom::CoinType, std::vector<std::string>>& addresses,
      base::OnceCallback<void(base::Time previous, base::Time current)>
          assets_last_discovered_at_test_fn,
      const base::Value::Dict& expected_next_asset_discovery_from_blocks,
      const std::vector<std::string>& expected_token_contract_addresses,
      std::vector<std::string> expected_chain_ids_remaining =
          GetAssetDiscoverySupportedEthChainsForTest()) {
    const base::Time previous_assets_last_discovered_at =
        GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::string& chain_id,
                const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
                absl::optional<mojom::ProviderError> error,
                const std::string& error_message) {
              expected_chain_ids_remaining.erase(
                  std::remove(expected_chain_ids_remaining.begin(),
                              expected_chain_ids_remaining.end(), chain_id),
                  expected_chain_ids_remaining.end());
            }));
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChainsRefresh(
        addresses);
    wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
        expected_token_contract_addresses);
    EXPECT_EQ(expected_chain_ids_remaining.size(), 0u);
    base::Time current_assets_last_discovered_at =
        GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
    std::move(assets_last_discovered_at_test_fn)
        .Run(previous_assets_last_discovered_at,
             current_assets_last_discovered_at);
    const base::Value::Dict current_next_asset_discovery_from_blocks =
        GetPrefs()->GetDict(kBraveWalletNextAssetDiscoveryFromBlocks).Clone();
    EXPECT_EQ(expected_next_asset_discovery_from_blocks,
              current_next_asset_discovery_from_blocks);
    wallet_service_observer_->Reset();
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

TEST_F(AssetDiscoveryManagerUnitTest, DiscoverEthAssets) {
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  std::string response;

  // Unsupported chainId is not supported
  TestDiscoverAssets(
      mojom::kBinanceSmartChainMainnetChainId, mojom::CoinType::ETH,
      {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {},
      mojom::ProviderError::kMethodNotSupported,
      l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR), "");

  // Empty address is invalid
  TestDiscoverAssets(mojom::kMainnetChainId, mojom::CoinType::ETH, {}, {},
                     mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     "");

  // Invalid address is invalid
  TestDiscoverAssets(mojom::kMainnetChainId, mojom::CoinType::ETH,
                     {"0xinvalid"}, {}, mojom::ProviderError::kInvalidParams,
                     l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                     "");

  // Invalid RPC response json response triggers parsing error
  auto expected_network =
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  std::string token_list_json = R"({
     "0x0d8775f648430679a709e98d2b0cb6250d2887ef": {
       "name": "Basic Attention Token",
       "logo": "bat.svg",
       "erc20": true,
       "symbol": "BAT",
       "decimals": 18
     }
    })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  SetInterceptor(expected_network, "invalid eth_getLogs response");
  TestDiscoverAssets(mojom::kMainnetChainId, mojom::CoinType::ETH,
                     {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {},
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), "");

  // Invalid limit exceeded response triggers parsing error
  SetLimitExceededJsonErrorResponse();
  TestDiscoverAssets(mojom::kMainnetChainId, mojom::CoinType::ETH,
                     {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {},
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), "");

  // Invalid logs (missing addresses) triggers parsing error
  response = R"({
    "jsonrpc": "2.0",
    "id": 1,
    "result": [
      {
        "blockHash": "0xaefb023131aa58e533c09c0eae29c280460d3976f5235a1ff53159ef37f73073",
        "blockNumber": "0xa72603",
        "data": "0x000000000000000000000000000000000000000000000006e83695ab1f893c00",
        "logIndex": "0x14",
        "removed": false,
        "topics": [
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000897bb1e945f5aa7ed7f81646e7991eaba63aa4b0",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash": "0x5c655301d386f45af116a4aef418491ee27b71ac30be70a593ccffa3754797d4",
        "transactionIndex": "0xa"
      },
    ]
  })";
  SetInterceptor(expected_network, response);
  TestDiscoverAssets(mojom::kMainnetChainId, mojom::CoinType::ETH,
                     {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {},
                     mojom::ProviderError::kParsingError,
                     l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR), "");

  // Valid registry token DAI is discovered and added.
  // Valid registry token WETH is discovered and added (tests insensitivity to
  // lower case addresses in provider logs response).
  // Valid BAT is not added because it is already a user asset.
  // Invalid LilNoun is not added because it is an ERC721.
  token_list_json = R"(
     {
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef": {
        "name": "Basic Attention Token",
        "logo": "bat.svg",
        "erc20": true,
        "symbol": "BAT",
        "decimals": 18
      },
      "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
        "name": "Dai Stablecoin",
        "logo": "dai.svg",
        "erc20": true,
        "symbol": "DAI",
        "decimals": 18
      },
      "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2": {
        "name": "Wrapped Eth",
        "logo": "weth.svg",
        "erc20": true,
        "symbol": "WETH",
        "decimals": 18,
        "chainId": "0x1"
      },
      "0x4b10701Bfd7BFEdc47d50562b76b436fbB5BdB3B": {
        "name": "Lil Nouns",
        "logo": "lilnouns.svg",
        "erc20": false,
        "erc721": true,
        "symbol": "LilNouns",
        "chainId": "0x1"
      }
     })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // Note: the matching transfer log for WETH uses an all lowercase address
  // while the token registry uses checksum address (contains uppercase)
  response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x6B175474E89094C44Da98b954EedeAC495271d0F",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464a",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      },
      {
        "address":"0x4b10701Bfd7BFEdc47d50562b76b436fbB5BdB3B",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464b",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      },
      {
        "address":"0xc02aaa39b223fe8d0a0e5c4f27ead9083c756cc2",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";

  SetInterceptor(expected_network, response);
  TestDiscoverAssets(mojom::kMainnetChainId, mojom::CoinType::ETH,
                     {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
                     {"0x6B175474E89094C44Da98b954EedeAC495271d0F",
                      "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"},
                     mojom::ProviderError::kSuccess, "", "0xd6464d");

  // Discover assets should not run unless using Infura proxy
  std::vector<base::Value::Dict> values;
  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x1");
  values.push_back(brave_wallet::NetworkInfoToValue(chain));
  UpdateCustomNetworks(GetPrefs(), &values);
  TestDiscoverAssets(
      mojom::kMainnetChainId, mojom::CoinType::ETH,
      {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, {},
      mojom::ProviderError::kMethodNotSupported,
      l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR),
      "0xd6464d");

  // Discover assets should be supported on Polygon
  token_list_json = R"(
    {
      "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
        "name": "Dai Stablecoin",
        "logo": "dai.svg",
        "erc20": true,
        "symbol": "DAI",
        "chainId": "0x89",
        "decimals": 18
      }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x6B175474E89094C44Da98b954EedeAC495271d0F",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  expected_network =
      GetNetwork(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH);
  SetInterceptor(expected_network, response);
  TestDiscoverAssets(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH,
                     {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
                     {"0x6B175474E89094C44Da98b954EedeAC495271d0F"},
                     mojom::ProviderError::kSuccess, "", "0xd6464d");

  // Discover assets should be supported on Optimism
  token_list_json = R"({
      "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
        "name": "Dai Stablecoin",
        "logo": "dai.svg",
        "erc20": true,
        "symbol": "DAI",
        "chainId": "0xa",
        "decimals": 18
      },
      "0x4200000000000000000000000000000000000006": {
        "name": "WETH optimism",
        "logo": "eth.svg",
        "erc20": true,
        "symbol": "WETH",
        "chainId": "0xa",
        "decimals": 18
      }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));
  response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x6B175474E89094C44Da98b954EedeAC495271d0F",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  expected_network =
      GetNetwork(mojom::kOptimismMainnetChainId, mojom::CoinType::ETH);
  SetInterceptor(expected_network, response);
  TestDiscoverAssets(mojom::kOptimismMainnetChainId, mojom::CoinType::ETH,
                     {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
                     {"0x6B175474E89094C44Da98b954EedeAC495271d0F"},
                     mojom::ProviderError::kSuccess, "", "0xd6464d");

  // Another Optimism asset discovered at later block
  // Asset discovered through block pref should update when newer transfers
  // for a given network are found
  response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x4200000000000000000000000000000000000006",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464d",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  SetInterceptor(expected_network, response);
  TestDiscoverAssets(mojom::kOptimismMainnetChainId, mojom::CoinType::ETH,
                     {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
                     {"0x4200000000000000000000000000000000000006"},
                     mojom::ProviderError::kSuccess, "", "0xd6464e");
}

TEST_F(AssetDiscoveryManagerUnitTest,
       DiscoverAssetsOnAllSupportedChainsAccountsAdded) {
  // Ethereum
  // Send valid requests that yield no results and verify
  // 1. OnDiscoverAssetsCompleted is called exactly once for each supported
  // chain
  // 2. All eth_getLogs requests specify all block ranges (earliest to latest)
  // 3. kBraveWalletNextAssetDiscoveryFromBlocks &
  // kBraveWalletAssetsLastDiscoveredAt prefs are not updated
  std::map<std::string, std::string> responses;
  std::map<std::string, std::string> expected_from_blocks;
  std::map<std::string, std::string> expected_to_blocks;
  const std::string default_response =
      R"({ "jsonrpc":"2.0", "id":1, "result":[] })";
  for (const std::string& supported_chain_id :
       GetAssetDiscoverySupportedEthChainsForTest()) {
    GURL network_url = GetNetwork(supported_chain_id, mojom::CoinType::ETH);
    responses[network_url.spec()] = default_response;
    expected_from_blocks[network_url.spec()] = kEthereumBlockTagEarliest;
    expected_to_blocks[network_url.spec()] = kEthereumBlockTagLatest;
  }
  const base::Time assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  auto expected_next_asset_discovery_from_blocks =
      GetPrefs()->GetDict(kBraveWalletNextAssetDiscoveryFromBlocks).Clone();
  SetDiscoverAssetsOnAllSupportedChainsInterceptor(
      responses, expected_from_blocks, expected_to_blocks);
  TestDiscoverAssetsOnAllSupportedChainsAccountsAdded(
      mojom::CoinType::ETH, {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
      assets_last_discovered_at, expected_next_asset_discovery_from_blocks);

  // Solana
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
  auto expected_network_url =
      GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
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
  TestDiscoverAssetsOnAllSupportedChainsAccountsAdded(
      mojom::CoinType::SOL, {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"},
      assets_last_discovered_at, expected_next_asset_discovery_from_blocks);
}

TEST_F(AssetDiscoveryManagerUnitTest,
       DiscoverAssetsOnAllSupportedChainsRefresh) {
  // Send valid requests that yield no results and verify
  // 1. OnDiscoverAssetsCompleted is called exactly once for each supported
  // chain
  // 2. kBraveWalletAssetsLastDiscoveredAt pref is updated
  // 3. kBraveWalletNextAssetDiscoveryFromBlocks pref is not updated
  const std::vector<std::string>& supported_chain_ids =
      GetAssetDiscoverySupportedEthChainsForTest();
  std::map<std::string, std::string> responses;
  std::map<std::string, std::string> expected_from_blocks;
  std::map<std::string, std::string> expected_to_blocks;
  std::string default_response = R"({ "jsonrpc":"2.0", "id":1, "result":[] })";
  for (const std::string& chain_id : supported_chain_ids) {
    GURL network_url = GetNetwork(chain_id, mojom::CoinType::ETH);
    responses[network_url.spec()] = default_response;
    expected_from_blocks[network_url.spec()] = kEthereumBlockTagEarliest;
    expected_to_blocks[network_url.spec()] = kEthereumBlockTagLatest;
  }

  std::map<mojom::CoinType, std::vector<std::string>> addresses = {
      {mojom::CoinType::ETH, {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}},
      {mojom::CoinType::SOL, {}}};

  base::Value::Dict expected_next_asset_discovery_from_blocks;  // Expect empty
  SetDiscoverAssetsOnAllSupportedChainsInterceptor(
      responses, expected_from_blocks, expected_to_blocks);
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_TRUE(previous < current);
      }),
      expected_next_asset_discovery_from_blocks, {});

  // Send valid requests that yield no results that are aborted because of rate
  // limiting rules and verify
  // 1. OnDiscoverAssetsCompleted is called exactly once for each supported
  // chain
  // 2. kBraveWalletLastDiscoveredAssetsAt stays the same
  // 3. kBraveWalletNextAssetDiscoveryFromBlocks stays the same (not
  // actually tested since the value is null...)
  expected_next_asset_discovery_from_blocks.clear();
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_EQ(previous, current);
      }),
      expected_next_asset_discovery_from_blocks, {}, {});

  // Send valid requests that yield new assets on multiple chains and verify
  // 1. OnDiscoverAssetsCompleted is called exactly once for each supported
  // chain
  // 2. Rate limit pref is updated
  // 3. Dict pref is is updated
  expected_next_asset_discovery_from_blocks.clear();
  responses.clear();
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));

  std::string token_list_json = R"({
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef": {
        "name": "Basic Attention Token",
        "logo": "bat.svg",
        "erc20": true,
        "symbol": "BAT",
        "chainId": "0x1",
        "decimals": 18
      },
      "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
        "name": "Dai Stablecoin",
        "logo": "dai.svg",
        "erc20": true,
        "symbol": "DAI",
        "chainId": "0x1",
        "decimals": 18
      },
      "0x4200000000000000000000000000000000000006": {
        "name": "WETH Optimism",
        "logo": "weth.svg",
        "erc20": true,
        "symbol": "WETH",
        "chainId": "0xa",
        "decimals": 18
      },
      "0x0000000000000000000000000000000000001010": {
        "name": "MATIC Polygon",
        "logo": "matic.svg",
        "erc20": true,
        "symbol": "WETH",
        "chainId": "0x89",
        "decimals": 18
      }})";

  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;

  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // Add responses for mainnet Ethereum, Optimism, then Polygon
  // Mainnet Ethereum
  GURL network_url = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  responses[network_url.spec()] = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464d",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  expected_from_blocks[network_url.spec()] = kEthereumBlockTagEarliest;
  expected_next_asset_discovery_from_blocks.SetByDottedPath(
      base::StrCat({kEthereumPrefKey, ".", mojom::kMainnetChainId}),
      "0xd6464e");

  // Optimism
  network_url =
      GetNetwork(mojom::kOptimismMainnetChainId, mojom::CoinType::ETH);
  responses[network_url.spec()] = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x4200000000000000000000000000000000000006",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464d",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  expected_from_blocks[network_url.spec()] = kEthereumBlockTagEarliest;
  expected_next_asset_discovery_from_blocks.SetByDottedPath(
      base::StrCat({kEthereumPrefKey, ".", mojom::kOptimismMainnetChainId}),
      "0xd6464e");

  // Polygon
  network_url = GetNetwork(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH);
  responses[network_url.spec()] = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x0000000000000000000000000000000000001010",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464d",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  expected_from_blocks[network_url.spec()] = kEthereumBlockTagEarliest;
  expected_next_asset_discovery_from_blocks.SetByDottedPath(
      base::StrCat({kEthereumPrefKey, ".", mojom::kPolygonMainnetChainId}),
      "0xd6464e");
  SetDiscoverAssetsOnAllSupportedChainsInterceptor(
      responses, expected_from_blocks, expected_to_blocks);
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_TRUE(previous < current);
      }),
      expected_next_asset_discovery_from_blocks,
      {"0x0000000000000000000000000000000000001010",
       "0x4200000000000000000000000000000000000006"});

  // Send valid request that for Ethereum mainnet (not rate limited) that yields
  // new assets at blocks after the previous largest block and verify
  // 1. from_block in eth_getLogs request is the
  // kBraveWalletNextAssetDiscoveryFromBlocks set from the previous
  // request Ethereum mainnet
  // 2. kBraveWalletNextAssetDiscoveryFromBlocks is incremented for
  // Ethereum mainnet
  expected_next_asset_discovery_from_blocks.clear();
  responses.clear();
  expected_from_blocks.clear();
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));
  GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);

  network_url = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  responses[network_url.spec()] = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x6B175474E89094C44Da98b954EedeAC495271d0F",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464e",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  expected_from_blocks[network_url.spec()] = "0xd6464e";
  expected_next_asset_discovery_from_blocks.SetByDottedPath(
      base::StrCat({kEthereumPrefKey, ".", mojom::kMainnetChainId}),
      "0xd6464f");
  expected_next_asset_discovery_from_blocks.SetByDottedPath(
      base::StrCat({kEthereumPrefKey, ".", mojom::kOptimismMainnetChainId}),
      "0xd6464e");
  expected_next_asset_discovery_from_blocks.SetByDottedPath(
      base::StrCat({kEthereumPrefKey, ".", mojom::kPolygonMainnetChainId}),
      "0xd6464e");

  SetDiscoverAssetsOnAllSupportedChainsInterceptor(
      responses, expected_from_blocks, expected_to_blocks);
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_TRUE(previous < current);
      }),
      expected_next_asset_discovery_from_blocks,
      {"0x6B175474E89094C44Da98b954EedeAC495271d0F"});
}

TEST_F(AssetDiscoveryManagerUnitTest,
       MultiChainDiscoverAssetsOnAllSupportedChainsRefresh) {
  // Add ETH assets to registry
  std::string eth_token_list = R"({
      "0x0d8775f648430679a709e98d2b0cb6250d2887ef": {
        "name": "Basic Attention Token",
        "logo": "bat.svg",
        "erc20": true,
        "symbol": "BAT",
        "chainId": "0x1",
        "decimals": 18
      },
      "0x6B175474E89094C44Da98b954EedeAC495271d0F": {
        "name": "Dai Stablecoin",
        "logo": "dai.svg",
        "erc20": true,
        "symbol": "DAI",
        "chainId": "0x1",
        "decimals": 18
      }})";
  auto* blockchain_registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(
      ParseTokenList(eth_token_list, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // Set up mock RPC responses with a map from chain_id -> mock response  (ETH
  // only)
  std::map<std::string, std::string> responses;
  std::map<std::string, std::string> expected_from_blocks;
  std::map<std::string, std::string> expected_to_blocks;
  const std::string default_response =
      R"({ "jsonrpc":"2.0", "id":1, "result":[] })";
  for (const std::string& supported_chain_id :
       GetAssetDiscoverySupportedEthChainsForTest()) {
    GURL network_url = GetNetwork(supported_chain_id, mojom::CoinType::ETH);
    responses[network_url.spec()] = default_response;
    expected_from_blocks[network_url.spec()] = kEthereumBlockTagEarliest;
    expected_to_blocks[network_url.spec()] = kEthereumBlockTagLatest;
  }

  // Create response for Ethereum eth_getLogs request
  GURL network_url = GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  responses[network_url.spec()] = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x6B175474E89094C44Da98b954EedeAC495271d0F",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464d",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  auto expected_next_asset_discovery_from_blocks =
      GetPrefs()->GetDict(kBraveWalletNextAssetDiscoveryFromBlocks).Clone();
  expected_next_asset_discovery_from_blocks.SetByDottedPath(
      base::StrCat({kEthereumPrefKey, ".", mojom::kMainnetChainId}),
      "0xd6464e");

  // Add SOL assets to registry
  std::string sol_token_list_json = R"({
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
  ASSERT_TRUE(ParseTokenList(sol_token_list_json, &token_list_map,
                             mojom::CoinType::SOL));
  for (auto& list_pair : token_list_map) {
    blockchain_registry->UpdateTokenList(list_pair.first,
                                         std::move(list_pair.second));
  }

  // Create response for Solana getTokenAccountsByOwner RPC request
  network_url = GetNetwork(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  responses[network_url.spec()] = R"({
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
  })";

  // Queue the responses for ETH and SOL
  std::map<mojom::CoinType, std::vector<std::string>> addresses = {
      {mojom::CoinType::ETH, {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}},
      {mojom::CoinType::SOL, {"4fzcQKyGFuk55uJaBZtvTHh42RBxbrZMuXzsGQvBJbwF"}}};
  SetDiscoverAssetsOnAllSupportedChainsInterceptor(
      responses, expected_from_blocks, expected_to_blocks);

  // Verify all three addresses (1 ETH and 2 SOL are discovered)
  // and that the next asset discovery from block is updated
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      addresses,
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_TRUE(previous < current);
      }),
      expected_next_asset_discovery_from_blocks,
      {"88j24JNwWLmJCjn2tZQ5jJzyaFtnusS2qsKup9NeDnd8",
       "EybFzCH4nBYEr7FD4wLWBvNZbEGgjy4kh584bGQntr1b",
       "0x6B175474E89094C44Da98b954EedeAC495271d0F"});
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
    },
    "0x4b10701Bfd7BFEdc47d50562b76b436fbB5BdB3B":{
      "name":"Lil Nouns",
      "logo":"lilnouns.svg",
      "erc20":false,
      "erc721":true,
      "symbol":"LilNouns",
      "chainId":"0x1"
    }, "0x6e84a6216eA6dACC71eE8E6b0a5B7322EEbC0fDd":{
      "name":"JoeToken",
      "logo":"joe.svg",
      "erc20":true,
      "symbol":"JOE",
      "decimals":18,
      "chainId":"0xa86a"
    },
    "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48":{
      "name":"USD Coin",
      "logo":"usdc.svg",
      "erc20":true,
      "symbol":"USDC",
      "decimals":18,
      "chainId":"0x1"
    },
    "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2":{
      "name":"Wrapped Eth",
      "logo":"weth.svg",
      "erc20":true,
      "symbol":"WETH",
      "decimals":18,
      "chainId":"0x1"
    },
    "0x03ab458634910aad20ef5f1c8ee96f1d6ac54919":{
      "name":"Rai Reflex Index",
      "logo":"rai.svg",
      "erc20":true,
      "symbol":"RAI",
      "decimals":18,
      "chainId":"0x1"
    }
  })";
  ASSERT_TRUE(
      ParseTokenList(token_list_json, &token_list_map, mojom::CoinType::ETH));
  blockchain_registry->UpdateTokenList(std::move(token_list_map));

  // RestoreWallet
  // Mock an eth_getLogs response that includes
  //  * DAI transfers to next account,
  //  0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db (valid)
  //  * JOE transfers to next account (invalid, wrong network)
  //  * LilNouns transfers to next account (invalid, not an ERC20 token)
  std::string response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x6b175474e89094c44da98b954eedeac495271d0f",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000f81229FE54D8a20fBc1e1e2a3451D1c7489437Db"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      },
      {
        "address":"0x6e84a6216eA6dACC71eE8E6b0a5B7322EEbC0fDd",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000f81229FE54D8a20fBc1e1e2a3451D1c7489437Db"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      },
      {
        "address":"0x4b10701Bfd7BFEdc47d50562b76b436fbB5BdB3B",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000f81229FE54D8a20fBc1e1e2a3451D1c7489437Db"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  auto intended_network =
      GetNetwork(mojom::kMainnetChainId, mojom::CoinType::ETH);
  SetInterceptor(intended_network, response);
  keyring_service_->RestoreWallet(
      kMnemonic1, kPasswordBrave, false,
      base::DoNothing());  // Creates account
                           // 0xf81229FE54D8a20fBc1e1e2a3451D1c7489437Db
  base::RunLoop().RunUntilIdle();
  std::vector<mojom::AccountInfoPtr> account_infos =
      keyring_service_->GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  EXPECT_EQ(account_infos.size(), 1u);
  std::vector<mojom::BlockchainTokenPtr> user_assets =
      BraveWalletService::GetUserAssets(mojom::kMainnetChainId,
                                        mojom::CoinType::ETH, GetPrefs());
  EXPECT_EQ(user_assets.size(), 3u);
  EXPECT_EQ(user_assets[user_assets.size() - 1]->symbol, "DAI");

  // AddAccount
  // Mock an eth_getLogs response that includes logs for WETH transfers to the
  // account to be added, 0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0
  response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x00000000000000000000000000c0f72E601C31DEb7890612cB92Ac0Fb7090EB0"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  SetInterceptor(intended_network, response);
  keyring_service_->AddAccount(
      "Account", mojom::CoinType::ETH,
      base::DoNothing());  // Creates account
                           // 0x00c0f72E601C31DEb7890612cB92Ac0Fb7090EB0
  base::RunLoop().RunUntilIdle();
  user_assets = BraveWalletService::GetUserAssets(
      mojom::kMainnetChainId, mojom::CoinType::ETH, GetPrefs());
  EXPECT_EQ(user_assets.size(), 4u);
  EXPECT_EQ(user_assets[user_assets.size() - 1]->symbol, "WETH");

  // AddHardwareAccounts
  // Mock an eth_getLogs response that includes logs for USDC transfers to the
  // hardware account to be added, 0x595a0583621FDe81A935021707e81343f75F9324
  response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000595a0583621FDe81A935021707e81343f75F9324"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  SetInterceptor(intended_network, response);
  std::vector<mojom::HardwareWalletAccountPtr> hardware_accounts;
  hardware_accounts.push_back(mojom::HardwareWalletAccount::New(
      "0x595a0583621FDe81A935021707e81343f75F9324", "m/44'/60'/1'/0/0",
      "name 1", "Ledger", "device1", mojom::CoinType::ETH, absl::nullopt));
  keyring_service_->AddHardwareAccounts(std::move(hardware_accounts));
  base::RunLoop().RunUntilIdle();
  account_infos =
      keyring_service_->GetAccountInfosForKeyring(mojom::kDefaultKeyringId);
  user_assets = BraveWalletService::GetUserAssets(
      mojom::kMainnetChainId, mojom::CoinType::ETH, GetPrefs());
  EXPECT_EQ(user_assets[user_assets.size() - 1]->symbol, "USDC");
  EXPECT_EQ(user_assets.size(), 5u);

  // ImportAccountForKeyring
  // Mock an eth_getLogs response that includes logs for RAI transfers to the
  // account to be imported, 0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266
  response = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[
      {
        "address":"0x03ab458634910aad20ef5f1c8ee96f1d6ac54919",
        "blockHash":"0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber":"0xd6464c",
        "data":"0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex":"0x159",
        "removed":false,
        "topics":[
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000f39Fd6e51aad88F6F4ce6aB8827279cffFb92266"
        ],
        "transactionHash":"0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex":"0x9f"
      }
    ]
  })";
  SetInterceptor(intended_network, response);
  const std::string private_key_str =
      "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
  std::vector<uint8_t> private_key_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(private_key_str, &private_key_bytes));
  ASSERT_TRUE(keyring_service_->ImportAccountForKeyring(
      mojom::kDefaultKeyringId, "Imported Account", private_key_bytes));
  base::RunLoop().RunUntilIdle();
  user_assets = BraveWalletService::GetUserAssets(
      mojom::kMainnetChainId, mojom::CoinType::ETH, GetPrefs());
  EXPECT_EQ(user_assets[user_assets.size() - 1]->symbol, "RAI");
  EXPECT_EQ(user_assets.size(), 6u);
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

}  // namespace brave_wallet
