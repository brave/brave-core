/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

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
  DictionaryPrefUpdate update(prefs, kBraveWalletCustomNetworks);
  base::Value* dict = update.Get();
  ASSERT_TRUE(dict);
  base::Value* list = dict->FindKey(kEthereumPrefKey);
  if (!list) {
    list = dict->SetKey(kEthereumPrefKey, base::Value(base::Value::Type::LIST));
  }
  ASSERT_TRUE(list);
  auto& list_value = list->GetList();
  list_value.clear();
  for (auto& it : *values) {
    list_value.Append(std::move(it));
  }
}

const std::vector<std::string>& GetAssetDiscoverySupportedChainsForTest() {
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
        GetAssetDiscoverySupportedChainsForTest());
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
          const auto expected_from_block =
              expected_from_blocks.find(request.url.spec())->second;
          const auto expected_to_block =
              expected_to_blocks.find(request.url.spec())->second;

          EXPECT_NE(request_string.find(base::StringPrintf(
                        "\"fromBlock\":\"%s\"", expected_from_block.c_str())),
                    std::string::npos);
          EXPECT_NE(request_string.find(base::StringPrintf(
                        "\"toBlock\":\"%s\"", expected_to_block.c_str())),
                    std::string::npos);
          url_loader_factory_.ClearResponses();
          for (const auto& kv : responses) {
            url_loader_factory_.AddResponse(kv.first, kv.second);
          }
          return;
        }));
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
    // 0 by the end of the DiscoverAssets call in order to trigger the
    // event and it will not be set by an outer
    // DiscoverAssetsOnAllSupportedChains* call in this unit test.
    asset_discovery_manager_->remaining_chains_ = 1;
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::string& chain_id,
                const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
                mojom::ProviderError error, const std::string& error_message) {
              EXPECT_EQ(chain_id, chain_id);
              EXPECT_EQ(expected_error, error);
              EXPECT_EQ(expected_error_message, error_message);
              ASSERT_EQ(expected_token_contract_addresses.size(),
                        discovered_assets.size());
              for (size_t i = 0; i < discovered_assets.size(); i++) {
                EXPECT_EQ(expected_token_contract_addresses[i],
                          discovered_assets[i]->contract_address);
              }
            }));
    asset_discovery_manager_->DiscoverAssets(
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
      const std::vector<std::string>& account_addresses,
      const base::Time expected_assets_last_discovered_at_pref,
      const base::Value::Dict& expected_next_asset_discovery_from_blocks,
      const std::vector<std::string>& expected_token_contract_addresses = {}) {
    std::vector<std::string> expected_chain_ids_remaining =
        GetAssetDiscoverySupportedChainsForTest();
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::string& chain_id,
                const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
                mojom::ProviderError error, const std::string& error_message) {
              expected_chain_ids_remaining.erase(
                  std::remove(expected_chain_ids_remaining.begin(),
                              expected_chain_ids_remaining.end(), chain_id),
                  expected_chain_ids_remaining.end());
            }));
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChainsAccountsAdded(
        account_addresses);
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
      const std::vector<std::string>& account_addresses,
      base::OnceCallback<void(base::Time previous, base::Time current)>
          assets_last_discovered_at_test_fn,
      const base::Value::Dict& expected_next_asset_discovery_from_blocks,
      const std::vector<std::string>& expected_token_contract_addresses,
      std::vector<std::string> expected_chain_ids_remaining =
          GetAssetDiscoverySupportedChainsForTest()) {
    const base::Time previous_assets_last_discovered_at =
        GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
    asset_discovery_manager_->SetDiscoverAssetsCompletedCallbackForTesting(
        base::BindLambdaForTesting(
            [&](const std::string& chain_id,
                const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
                mojom::ProviderError error, const std::string& error_message) {
              expected_chain_ids_remaining.erase(
                  std::remove(expected_chain_ids_remaining.begin(),
                              expected_chain_ids_remaining.end(), chain_id),
                  expected_chain_ids_remaining.end());
            }));
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChainsRefresh(
        account_addresses);
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

TEST_F(AssetDiscoveryManagerUnitTest, DiscoverAssets) {
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
       GetAssetDiscoverySupportedChainsForTest()) {
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
      {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"}, assets_last_discovered_at,
      expected_next_asset_discovery_from_blocks);
}

TEST_F(AssetDiscoveryManagerUnitTest,
       DiscoverAssetsOnAllSupportedChainsRefresh) {
  // Send valid requests that yield no results and verify
  // 1. OnDiscoverAssetsCompleted is called exactly once for each supported
  // chain
  // 2. kBraveWalletAssetsLastDiscoveredAt pref is updated
  // 3. kBraveWalletNextAssetDiscoveryFromBlocks pref is not updated
  const std::vector<std::string>& supported_chain_ids =
      GetAssetDiscoverySupportedChainsForTest();
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

  base::Value::Dict expected_next_asset_discovery_from_blocks;  // Expect empty
  SetDiscoverAssetsOnAllSupportedChainsInterceptor(
      responses, expected_from_blocks, expected_to_blocks);
  TestDiscoverAssetsOnAllSupportedChainsRefresh(
      {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
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
      {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
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
      {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
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
      {"0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961"},
      base::BindLambdaForTesting([&](base::Time previous, base::Time current) {
        EXPECT_TRUE(previous < current);
      }),
      expected_next_asset_discovery_from_blocks,
      {"0x6B175474E89094C44Da98b954EedeAC495271d0F"});
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
    },
    "0x6e84a6216eA6dACC71eE8E6b0a5B7322EEbC0fDd":{
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

}  // namespace brave_wallet
