/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer_base.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
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
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class TestBraveWalletServiceObserverForAssetDiscoveryManager
    : public brave_wallet::BraveWalletServiceObserverBase {
 public:
  TestBraveWalletServiceObserverForAssetDiscoveryManager() = default;

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
    run_loop_asset_discovery_->Quit();
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
    wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_,
        BraveWalletServiceDelegate::Create(profile_.get()), GetPrefs(),
        GetLocalState());
    network_manager_ = wallet_service_->network_manager();
    json_rpc_service_ = wallet_service_->json_rpc_service();
    keyring_service_ = wallet_service_->keyring_service();
    tx_service_ = wallet_service_->tx_service();
    simple_hash_client_ =
        std::make_unique<SimpleHashClient>(shared_url_loader_factory_);
    asset_discovery_manager_ = std::make_unique<AssetDiscoveryManager>(
        shared_url_loader_factory_, *wallet_service_, *json_rpc_service_,
        *keyring_service_, *simple_hash_client_, GetPrefs());
    wallet_service_observer_ = std::make_unique<
        TestBraveWalletServiceObserverForAssetDiscoveryManager>();
    wallet_service_->AddObserver(wallet_service_observer_->GetReceiver());

    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            TRAFFIC_ANNOTATION_FOR_TESTS, shared_url_loader_factory_);
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  TestingPrefServiceSimple* GetLocalState() { return local_state_->Get(); }
  GURL GetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    return network_manager_->GetNetworkURL(chain_id, coin);
  }
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<TestBraveWalletServiceObserverForAssetDiscoveryManager>
      wallet_service_observer_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveWalletService> wallet_service_;
  std::unique_ptr<SimpleHashClient> simple_hash_client_;
  std::unique_ptr<AssetDiscoveryManager> asset_discovery_manager_;
  raw_ptr<NetworkManager> network_manager_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<TxService> tx_service_;
  base::test::ScopedFeatureList scoped_feature_list_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;

  void TestDiscoverAssetsOnAllSupportedChains(
      const std::map<mojom::CoinType, std::vector<std::string>>&
          account_addresses,
      bool bypass_rate_limit,
      bool expect_events_fired,
      const std::vector<std::string>& expected_token_contract_addresses,
      size_t expected_ending_queue_size = 0u) {
    asset_discovery_manager_->DiscoverAssetsOnAllSupportedChains(
        account_addresses, bypass_rate_limit);
    if (expect_events_fired) {
      wallet_service_observer_->WaitForOnDiscoverAssetsCompleted(
          expected_token_contract_addresses);
      EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsStartedFired());
      EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
    } else {
      task_environment_.RunUntilIdle();
      EXPECT_FALSE(wallet_service_observer_->OnDiscoverAssetsStartedFired());
      EXPECT_FALSE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
    }
    EXPECT_EQ(asset_discovery_manager_->GetQueueSizeForTesting(),
              expected_ending_queue_size);
    wallet_service_observer_->Reset();
  }
};

TEST_F(AssetDiscoveryManagerUnitTest, GetFungibleSupportedChains) {
  // GetFungibleSupportedChains should return a map of the same size
  // vectors every time
  const std::map<mojom::CoinType, std::vector<std::string>> chains1 =
      asset_discovery_manager_->GetFungibleSupportedChains();
  const std::map<mojom::CoinType, std::vector<std::string>> chains2 =
      asset_discovery_manager_->GetFungibleSupportedChains();
  const std::map<mojom::CoinType, std::vector<std::string>> chains3 =
      asset_discovery_manager_->GetFungibleSupportedChains();
  EXPECT_GT(chains1.at(mojom::CoinType::ETH).size(), 0u);
  EXPECT_GT(chains1.at(mojom::CoinType::SOL).size(), 0u);

  EXPECT_EQ(chains2.at(mojom::CoinType::ETH).size(),
            chains1.at(mojom::CoinType::ETH).size());
  EXPECT_EQ(chains2.at(mojom::CoinType::SOL).size(),
            chains1.at(mojom::CoinType::SOL).size());
}

TEST_F(AssetDiscoveryManagerUnitTest, GetNonFungibleSupportedChains) {
  // Gnosis chain ID should not be included if it's not a custom network
  std::map<mojom::CoinType, std::vector<std::string>> chains =
      asset_discovery_manager_->GetNonFungibleSupportedChains();
  EXPECT_EQ(chains.at(mojom::CoinType::ETH).size(), 6UL);
  EXPECT_EQ(chains.at(mojom::CoinType::SOL).size(), 1UL);

  // Verify none of the chain IDs == mojom::kGnosisChainId
  EXPECT_EQ(
      std::find(chains.at(mojom::CoinType::ETH).begin(),
                chains.at(mojom::CoinType::ETH).end(), mojom::kGnosisChainId),
      chains.at(mojom::CoinType::ETH).end());

  // Add a custom network (Gnosis) and verify it is included
  auto gnosis_network = GetTestNetworkInfo1(mojom::kGnosisChainId);
  network_manager_->AddCustomNetwork(gnosis_network);

  chains = asset_discovery_manager_->GetNonFungibleSupportedChains();
  EXPECT_EQ(chains.at(mojom::CoinType::ETH).size(), 7UL);
  EXPECT_EQ(chains.at(mojom::CoinType::SOL).size(), 1UL);

  // Verify one of the chain IDs is mojom::kGnosisChainId
  EXPECT_NE(
      std::find(chains.at(mojom::CoinType::ETH).begin(),
                chains.at(mojom::CoinType::ETH).end(), mojom::kGnosisChainId),
      chains.at(mojom::CoinType::ETH).end());
}

TEST_F(AssetDiscoveryManagerUnitTest, DiscoverAssetsOnAllSupportedChains) {
  // Verify OnDiscoverAssetsStarted and OnDiscoverAssetsCompleted both fire and
  // kBraveWalletLastDiscoveredAssetsAt does not update if accounts_added set
  base::Time current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  ASSERT_EQ(current_assets_last_discovered_at, base::Time());
  TestDiscoverAssetsOnAllSupportedChains({}, true, true, {});
  base::Time previous_assets_last_discovered_at =
      current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_EQ(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // Verify OnDiscoverAssetsStarted and OnDiscoverAssetsCompleted both fire and
  // kBraveWalletLastDiscoveredAssetsAt updates if accounts_added not set
  TestDiscoverAssetsOnAllSupportedChains({}, false, true, {});
  previous_assets_last_discovered_at = current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_GT(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // Verify subsequent requests are throttled if within the rate limiting window
  // and accounts_added not set
  TestDiscoverAssetsOnAllSupportedChains({}, false, false, {});
  previous_assets_last_discovered_at = current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_EQ(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // Verify subsequent requests are not throttled if within the rate limiting
  // window and accounts added set
  TestDiscoverAssetsOnAllSupportedChains({}, true, true, {});

  // Verify once the rate limiting window has passed, subsequent requests are
  // not rate limited if accounts added not set
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));
  TestDiscoverAssetsOnAllSupportedChains({}, false, true, {});
  previous_assets_last_discovered_at = current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_GT(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);

  // If there is already a task in-flight, OnDiscoverAssetsStarted and
  // OnDiscoverAssetsCompleted both fire if accounts_added is set.
  task_environment_.FastForwardBy(
      base::Minutes(kAssetDiscoveryMinutesPerRequest));
  std::queue<std::unique_ptr<AssetDiscoveryTask>> tasks;
  tasks.push(std::make_unique<AssetDiscoveryTask>(
      *api_request_helper_, *simple_hash_client_, *wallet_service_,
      *json_rpc_service_, GetPrefs()));
  asset_discovery_manager_->SetQueueForTesting(std::move(tasks));
  TestDiscoverAssetsOnAllSupportedChains({}, true, true, {}, 1);

  // If there is already a task in-flight, nothing is run
  // if accounts_added not set.
  TestDiscoverAssetsOnAllSupportedChains({}, false, false, {}, 1);
}

// KeyringServiceObserver test
TEST_F(AssetDiscoveryManagerUnitTest, AccountsAdded) {
  // Verifies that the AssetDiscoveryManager is added as an observer to the
  // KeyringService, and that discovery is run when new accounts are added
  base::Time current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  ASSERT_EQ(current_assets_last_discovered_at, base::Time());
  ASSERT_TRUE(keyring_service_->RestoreWalletSync(kMnemonicDivideCruise,
                                                  kTestWalletPassword, false));
  wallet_service_observer_->WaitForOnDiscoverAssetsCompleted({});
  base::Time previous_assets_last_discovered_at =
      current_assets_last_discovered_at;
  current_assets_last_discovered_at =
      GetPrefs()->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsStartedFired());
  EXPECT_TRUE(wallet_service_observer_->OnDiscoverAssetsCompletedFired());
  EXPECT_EQ(current_assets_last_discovered_at,
            previous_assets_last_discovered_at);
  wallet_service_observer_->Reset();
}

}  // namespace brave_wallet
