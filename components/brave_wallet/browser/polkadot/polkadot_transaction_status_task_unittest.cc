/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_transaction_status_task.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// This extrinsic lives here at block:
// https://westend.subscan.io/block/30277984
inline constexpr char kDefaultExtrinsic[] =
    R"(3d02840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c015c06ae85d9b151a09c12627b580a3fb991c92662c5cc7feeca73833868f83756c7489aaacd6e11e2f3d7f15b34e44c3bca39282a70b848d9a674e600cd11c78bc5018c00000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a29a5100)";

void RegisterDefaultFinalizedHeader(PolkadotMockRpc* polkadot_mock_rpc) {
  polkadot_mock_rpc->SetFinalizedBlockHeader(R"(
    {
      "jsonrpc":"2.0",
      "id":1,
      "result":{
        "parentHash":"0x3abc27144bbf0c75c90335c42438e365aa4b7e9188b8a980d9a1d4d32f2b44fa",
        "number":"0x1ce354f",
        "stateRoot":"0x4a0be8dd0e559d1db6a3706cd680ab725bff879a573e55944b24c9fdadd2a0c8",
        "extrinsicsRoot":"0xf392f83f6fe0a75a468b4f59a241377cf28a6d5a892d936b6bb84c213d74fa0d",
        "digest":{
          "logs":[
            "0x0642414245b5010302000000b5659f11000000009abf4ca7b5e17b0c8dd71723a5c9f26b856d98e855c4d2153024b872a83d3a08a4b15a23ffae388ac0e3e71787079e6afa86e5f1db4733fa88abe1f8d536bd04cfcafb33306d0d8dc933c0c42dcdf5af5b7e55829b1c9f2993a9d1f668e53106",
            "0x0442454546840385b89b5af093f2269fac4180942cd2f3f3814783f33ee572b3c6df4570e21fc1",
            "0x054241424501017c737c5a03844ffdf92efbe9a8fa2c47f40d9b3b4a040ef99c1ed3ba4529916a59611765c8d78e4d4ba9c8eb4866aa89e5bdf9959583de8a073696134d107782"
          ]
        }
      }
    }
  )");
}

void RegisterDefaultBlockHashes(PolkadotMockRpc* polkadot_mock_rpc) {
  base::flat_map<uint32_t, std::string> block_hash_map;
  block_hash_map.emplace(
      30277982,
      "0x411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2");
  block_hash_map.emplace(
      30277983,
      "0xf6f199e59c1362237dd801d2748274a8ffff0416677b5e2c9bf01d5c7114c759");
  block_hash_map.emplace(
      30277984,
      "0x14a9e3880ed1d2b7cd6ab0480ea54ab0cde8d1cb0cef92cba7d588ae32331d61");
  block_hash_map.emplace(
      30277985,
      "0xf493ed96279f66793b771ddbebf26fe22aa1a7684c18d3e4ef508a5a180ff082");
  block_hash_map.emplace(
      30277986,
      "0x781d716c411c2c8585813cde22cc5a4781889b46f3c12719788c4ffae0051bd8");

  polkadot_mock_rpc->SetBlockHashMap(std::move(block_hash_map));
}

void RegisterDefaultBlocks(PolkadotMockRpc* polkadot_mock_rpc) {
  PolkadotBlock block;

  ASSERT_TRUE(base::HexStringToSpan(
      "f6f199e59c1362237dd801d2748274a8ffff0416677b5e2c9bf01d5c7114c759",
      block.header.parent_hash));

  block.header.block_number = 30277984;
  ASSERT_TRUE(base::HexStringToSpan(
      "3714872aeb0e9e2c74e3e18d246d49c8b49b462e71cd9240dbf8621bb3b00d5b",
      block.header.state_root));

  ASSERT_TRUE(base::HexStringToSpan(
      "d165ba265ae402abafa8e734da62c6e77bc9f3f64a82e8ee9f99478f5d5e1c3c",
      block.header.extrinsics_root));

  ASSERT_TRUE(base::HexStringToBytes(
      R"(0c)"
      R"(0642414245b5010101000000c6319f1100000000f41b9118cfd7371a3e158bbca19f2554aa973c418d5f4a0dbf2ed1904de28d33d2ecff40a1fb7b7d772a379756190de73dd1434656cb80996fbbdc7a4e1f330b2e78028163dbb29f86584c096520bd9d0925e6ad2e12392afbe784bbdb30260c)"
      R"(04424545468403d30bb0cfed49f8e283c82e5c2c1c2312d1ee97dda7b5d8834ddded40776d8ec9)"
      R"(054241424501016a387fefdcb284dae1726dbe937c8d869d68cabbda431d91ad78297c2ea06e0675b899c27604f5fe312e248ab5dad601b17fcb584a82bba8b8ad908f6937678b)",
      &block.header.encoded_logs));

  block.extrinsics = {
      R"(0x1234)", R"(0xabcd)",
      R"(0x3d02840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c015c06ae85d9b151a09c12627b580a3fb991c92662c5cc7feeca73833868f83756c7489aaacd6e11e2f3d7f15b34e44c3bca39282a70b848d9a674e600cd11c78bc5018c00000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a29a5100)"};

  base::flat_map<std::string, PolkadotBlock> block_map;
  block_map.emplace(
      "411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2",
      PolkadotBlock{});
  block_map.emplace(
      "f6f199e59c1362237dd801d2748274a8ffff0416677b5e2c9bf01d5c7114c759",
      PolkadotBlock{});
  block_map.emplace(
      "14a9e3880ed1d2b7cd6ab0480ea54ab0cde8d1cb0cef92cba7d588ae32331d61",
      std::move(block));
  block_map.emplace(
      "f493ed96279f66793b771ddbebf26fe22aa1a7684c18d3e4ef508a5a180ff082",
      PolkadotBlock{});
  block_map.emplace(
      "781d716c411c2c8585813cde22cc5a4781889b46f3c12719788c4ffae0051bd8",
      PolkadotBlock{});

  polkadot_mock_rpc->SetBlockMap(std::move(block_map));
}

void RegisterCustomEventsForDefaultBlock(PolkadotMockRpc* polkadot_mock_rpc,
                                         std::string_view events_hex) {
  base::flat_map<std::string, std::string> events_map;

  events_map.emplace(
      // This block hash doesn't actually match the "real" block hash because
      // our JSON encoding routines don't include the logs. This is because we
      // need an array of logs but we store a single flat std::string of
      // encoded logs, which would mean we'd need to split it up to properly
      // build the JSON.
      "1623fa73ce7855d3eb51c9eb274b58322c03cb09347cbb924bcb8b63ac335cd9",
      events_hex);

  polkadot_mock_rpc->SetEventsMap(std::move(events_map));
}

}  // namespace

class PolkadotTransactionStatusTaskUnitTest : public testing::Test {
 public:
  PolkadotTransactionStatusTaskUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~PolkadotTransactionStatusTaskUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletPolkadotFeature);

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);

    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    polkadot_testnet_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotTestnet, 0);
    polkadot_mainnet_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotMainnet, 0);
    ASSERT_TRUE(polkadot_testnet_account_);
  }

  void UnlockWallet() {
    base::test::TestFuture<bool> future;
    keyring_service_->Unlock(kTestWalletPassword, future.GetCallback());
    ASSERT_TRUE(future.Take());
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;
  mojom::AccountInfoPtr polkadot_testnet_account_;
  mojom::AccountInfoPtr polkadot_mainnet_account_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;

  network::TestURLLoaderFactory url_loader_factory_;

  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
};

TEST_F(PolkadotTransactionStatusTaskUnitTest, FindExtrinsicSucceeds) {
  // Test that a "cold" transaction can trivially have its status be found,
  // where "cold" is defined as not outpacing the finalized head of the chain.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  RegisterDefaultFinalizedHeader(polkadot_mock_rpc.get());
  RegisterDefaultBlockHashes(polkadot_mock_rpc.get());
  RegisterDefaultBlocks(polkadot_mock_rpc.get());
  RegisterCustomEventsForDefaultBlock(
      polkadot_mock_rpc.get(),
      R"(0b080000000e000000000001000000000007d41643ad0b997002000000020000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b50300000000000000000000000000020000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95168eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a866140000000000000000000000000000000200000004076d6f646c6461702f7361746c0000000000000000000000000000000000000000dc8df1b50300000000000000000000000000020000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b5030000000000000000000000000000000000000000000000000000000000020000000000823798916da8000000)");

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());

  const auto [status, fee] = *result;
  EXPECT_EQ(status, PolkadotTransactionStatus::kSuccess);
  EXPECT_EQ(fee, uint128_t{15937408476});
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, FindExtrinsicFailed) {
  // Test can we find a failed extrinsic.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  RegisterDefaultFinalizedHeader(polkadot_mock_rpc.get());
  RegisterDefaultBlockHashes(polkadot_mock_rpc.get());
  RegisterDefaultBlocks(polkadot_mock_rpc.get());
  RegisterCustomEventsForDefaultBlock(
      polkadot_mock_rpc.get(),
      R"(0b080000000e000000000001000000000007d41643ad0b997002000000020000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b50300000000000000000000000000020000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95168eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a866140000000000000000000000000000000200000004076d6f646c6461702f7361746c0000000000000000000000000000000000000000dc8df1b50300000000000000000000000000020000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b5030000000000000000000000000000000000000000000000000000000000020000000001823798916da8000000)");

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());

  const auto [status, fee] = *result;
  EXPECT_EQ(status, PolkadotTransactionStatus::kFailed);
  EXPECT_EQ(fee, uint128_t{15937408476});
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, InconsistentFees) {
  // Test that we error out if the fees paid don't match the fees withdrawn.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  RegisterDefaultFinalizedHeader(polkadot_mock_rpc.get());
  RegisterDefaultBlockHashes(polkadot_mock_rpc.get());
  RegisterDefaultBlocks(polkadot_mock_rpc.get());
  RegisterCustomEventsForDefaultBlock(
      polkadot_mock_rpc.get(),
      R"(0b080000000e000000000001000000000007d41643ad0b997002000000020000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b50300000000000000000000000000020000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95168eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a866140000000000000000000000000000000200000004076d6f646c6461702f7361746c0000000000000000000000000000000000000000dc8df1b50300000000000000000000000000020000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b5130000000000000000000000000000000000000000000000000000000000020000000000823798916da8000000)");

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());

  const auto [status, fee] = *result;
  EXPECT_EQ(status, PolkadotTransactionStatus::kInvalidResponse);
  EXPECT_EQ(fee, std::nullopt);
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, ExtrinsicNotInMortalityWindow) {
  // Test that if an extrinsic is not found within its mortality window that we
  // mark it as not found.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 2;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  RegisterDefaultFinalizedHeader(polkadot_mock_rpc.get());
  RegisterDefaultBlockHashes(polkadot_mock_rpc.get());
  RegisterDefaultBlocks(polkadot_mock_rpc.get());

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());

  const auto [status, fee] = *result;
  EXPECT_EQ(status, PolkadotTransactionStatus::kNotFound);
  EXPECT_EQ(fee, std::nullopt);
}

TEST_F(PolkadotTransactionStatusTaskUnitTest,
       ExtrinsicNotInMortalityWindow_NumericLimits) {
  // Test that if an extrinsic is not found within its mortality window that we
  // mark it as not found, this time exercising end-of-chain numeric limits.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t num_blocks = 13;
  const uint32_t block_num = std::numeric_limits<uint32_t>::max() - num_blocks;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  // Note how the block number is large.
  polkadot_mock_rpc->SetFinalizedBlockHeader(R"(
    {
      "jsonrpc":"2.0",
      "id":1,
      "result":{
        "parentHash":"0x3abc27144bbf0c75c90335c42438e365aa4b7e9188b8a980d9a1d4d32f2b44fa",
        "number":"0xffffffff",
        "stateRoot":"0x4a0be8dd0e559d1db6a3706cd680ab725bff879a573e55944b24c9fdadd2a0c8",
        "extrinsicsRoot":"0xf392f83f6fe0a75a468b4f59a241377cf28a6d5a892d936b6bb84c213d74fa0d",
        "digest":{
          "logs":[
            "0x0642414245b5010302000000b5659f11000000009abf4ca7b5e17b0c8dd71723a5c9f26b856d98e855c4d2153024b872a83d3a08a4b15a23ffae388ac0e3e71787079e6afa86e5f1db4733fa88abe1f8d536bd04cfcafb33306d0d8dc933c0c42dcdf5af5b7e55829b1c9f2993a9d1f668e53106",
            "0x0442454546840385b89b5af093f2269fac4180942cd2f3f3814783f33ee572b3c6df4570e21fc1",
            "0x054241424501017c737c5a03844ffdf92efbe9a8fa2c47f40d9b3b4a040ef99c1ed3ba4529916a59611765c8d78e4d4ba9c8eb4866aa89e5bdf9959583de8a073696134d107782"
          ]
        }
      }
    }
  )");

  // For this test, we'll just reuse the same blockhash across all requests and
  // we'll return an empty block each time. This guarantees the extrinsic won't
  // be found within the mortality period.
  {
    base::flat_map<uint32_t, std::string> block_hash_map;
    for (uint32_t i = 0; i < num_blocks; ++i) {
      block_hash_map.emplace(
          block_num + i,
          "0x411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2");
    }

    polkadot_mock_rpc->SetBlockHashMap(std::move(block_hash_map));
  }

  {
    base::flat_map<std::string, PolkadotBlock> block_map;
    block_map.emplace(
        "411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2",
        PolkadotBlock{});

    polkadot_mock_rpc->SetBlockMap(std::move(block_map));
  }

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());

  const auto [status, fee] = *result;
  EXPECT_EQ(status, PolkadotTransactionStatus::kNotFound);
  EXPECT_EQ(fee, std::nullopt);
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, OutpacedFinalizedHead) {
  // Test that our task terminates early if we outpace the finalized head.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  // Note here the block number value of 0x01ce0161 which is 30277985 which is 3
  // added to 30277982, our anchor block. In total, we should 4 requests here:
  // {82,82,84,85}, i.e. we include the finalized block as part of our search.
  polkadot_mock_rpc->SetFinalizedBlockHeader(R"(
    {
      "jsonrpc":"2.0",
      "id":1,
      "result":{
        "parentHash":"0x3abc27144bbf0c75c90335c42438e365aa4b7e9188b8a980d9a1d4d32f2b44fa",
        "number":"0x1ce0161",
        "stateRoot":"0x4a0be8dd0e559d1db6a3706cd680ab725bff879a573e55944b24c9fdadd2a0c8",
        "extrinsicsRoot":"0xf392f83f6fe0a75a468b4f59a241377cf28a6d5a892d936b6bb84c213d74fa0d",
        "digest":{
          "logs":[
            "0x0642414245b5010302000000b5659f11000000009abf4ca7b5e17b0c8dd71723a5c9f26b856d98e855c4d2153024b872a83d3a08a4b15a23ffae388ac0e3e71787079e6afa86e5f1db4733fa88abe1f8d536bd04cfcafb33306d0d8dc933c0c42dcdf5af5b7e55829b1c9f2993a9d1f668e53106",
            "0x0442454546840385b89b5af093f2269fac4180942cd2f3f3814783f33ee572b3c6df4570e21fc1",
            "0x054241424501017c737c5a03844ffdf92efbe9a8fa2c47f40d9b3b4a040ef99c1ed3ba4529916a59611765c8d78e4d4ba9c8eb4866aa89e5bdf9959583de8a073696134d107782"
          ]
        }
      }
    }
  )");

  // For this test, we'll just reuse the same blockhash across all requests and
  // we'll return an empty block each time. This guarantees the extrinsic won't
  // be found and we should outpace the finalized head quickly. We should only
  // need 4 blocks here, as our finalized head should be included.
  {
    base::flat_map<uint32_t, std::string> block_hash_map;
    for (uint32_t i = 0; i < 4; ++i) {
      block_hash_map.emplace(
          30277982 + i,
          "0x411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2");
    }

    polkadot_mock_rpc->SetBlockHashMap(std::move(block_hash_map));
  }

  {
    base::flat_map<std::string, PolkadotBlock> block_map;
    block_map.emplace(
        "411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2",
        PolkadotBlock{});

    polkadot_mock_rpc->SetBlockMap(std::move(block_map));
  }

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());

  const auto [status, fee] = *result;
  EXPECT_EQ(status, PolkadotTransactionStatus::kNotFinalized);
  EXPECT_EQ(fee, std::nullopt);
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, NetworkFailure_NoChainMetadata) {
  // Test that our task cleanly terminates if ChainMetadata isn't available.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  polkadot_mock_rpc->UseInvalidChainMetadata();
  polkadot_mock_rpc->FinalizeSetup();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotTransactionStatusTaskUnitTest,
       NetworkFailure_NoFinalizedBlockHash) {
  // Test that our task cleanly terminates if acquiring the block hash of the
  // latest canon block fails.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  polkadot_mock_rpc->UseInvalidFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->FinalizeSetup();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), WalletParsingErrorMessage());
}

TEST_F(PolkadotTransactionStatusTaskUnitTest,
       NetworkFailure_NoFinalizedBlockHeader) {
  // Test that our task cleanly terminates if acquiring the header of the
  // finalized block fails.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  polkadot_mock_rpc->SetFinalizedBlockHeader(R"(
    {
      "jsonrpc":"2.0",
      "id":1,
      "result":"0xcat!"
    }
  )");
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->FinalizeSetup();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), WalletParsingErrorMessage());
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, NetworkFailure_NoBlockHash) {
  // Test that our task can handle a block hash request failing.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  RegisterDefaultFinalizedHeader(polkadot_mock_rpc.get());

  {
    // For this test, we're trying to simulate an error path in the handling of
    // the block hash requests. On the fourth request for a blockhash, return
    // something nonsensical to exercise the error path.
    base::flat_map<uint32_t, std::string> block_hash_map;
    for (uint32_t i = 0; i < 64; ++i) {
      const char* block_hash =
          "0x411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2";

      if (i == 3) {
        block_hash = R"(0xcat!!!!!!!)";
      }

      block_hash_map.emplace(30277982 + i, block_hash);
    }

    polkadot_mock_rpc->SetBlockHashMap(std::move(block_hash_map));
  }

  {
    base::flat_map<std::string, PolkadotBlock> block_map;
    block_map.emplace(
        "411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2",
        PolkadotBlock{});

    polkadot_mock_rpc->SetBlockMap(std::move(block_map));
  }

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), WalletParsingErrorMessage());
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, NetworkFailure_NoBlock) {
  // Test that our task can handle a block request failing.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  RegisterDefaultFinalizedHeader(polkadot_mock_rpc.get());

  {
    base::flat_map<uint32_t, std::string> block_hash_map;
    for (uint32_t i = 0; i < 64; ++i) {
      const char* block_hash =
          "0x411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2";

      if (i == 3) {
        block_hash =
            R"(0x14a9e3880ed1d2b7cd6ab0480ea54ab0cde8d1cb0cef92cba7d588ae32331d61)";
      }

      block_hash_map.emplace(30277982 + i, block_hash);
    }

    polkadot_mock_rpc->SetBlockHashMap(std::move(block_hash_map));
  }

  {
    base::flat_map<std::string, PolkadotBlock> block_map;
    block_map.emplace(
        "411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2",
        PolkadotBlock{});

    block_map.emplace(
        "14a9e3880ed1d2b7cd6ab0480ea54ab0cde8d1cb0cef92cba7d588ae32331d61",
        PolkadotBlock{});

    polkadot_mock_rpc->SetBadBlockMapKey(
        "14a9e3880ed1d2b7cd6ab0480ea54ab0cde8d1cb0cef92cba7d588ae32331d61");
    polkadot_mock_rpc->SetBlockMap(std::move(block_map));
  }

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), WalletParsingErrorMessage());
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, NetworkFailure_NoEvents) {
  // Test that our task can handle a request for a block's events failing.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t block_num = 30277982;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future;

  RegisterDefaultFinalizedHeader(polkadot_mock_rpc.get());
  RegisterDefaultBlockHashes(polkadot_mock_rpc.get());
  RegisterDefaultBlocks(polkadot_mock_rpc.get());
  RegisterCustomEventsForDefaultBlock(polkadot_mock_rpc.get(), R"(0xcat!!!!)");

  polkadot_mock_rpc->AddGetFinalizedBlockHash();
  polkadot_mock_rpc->AddGetFinalizedBlockHeader();
  polkadot_mock_rpc->FinalizeSetup();

  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), WalletParsingErrorMessage());
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, ConcurrentStartRejected) {
  // Test that users get an error if they attempt to call Start() twice on the
  // same task.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t num_blocks = 13;
  const uint32_t block_num = std::numeric_limits<uint32_t>::max() - num_blocks;
  const uint32_t mortality_period = 64;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_TRUE(task);

  base::test::TestFuture<base::expected<
      std::pair<PolkadotTransactionStatus, std::optional<uint128_t>>,
      std::string>>
      future1, future2;

  task->Start(future1.GetCallback());
  task->Start(future2.GetCallback());

  auto result = future2.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotTransactionStatusTaskUnitTest, RejectLargeMortality) {
  // Test that we don't create a task if the provided mortality period exceeds
  // something reasonable.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::vector<uint8_t> extrinsic;
  ASSERT_TRUE(base::HexStringToBytes(kDefaultExtrinsic, &extrinsic));

  // To test our block walking routines, we deliberately choose a block before
  // the one where our extrinsic actually lives.
  const uint32_t num_blocks = 13;
  const uint32_t block_num = std::numeric_limits<uint32_t>::max() - num_blocks;
  const uint32_t mortality_period = 2 * 1024;

  auto task = PolkadotTransactionStatusTask::Create(
      *polkadot_wallet_service, *keyring_service_,
      polkadot_testnet_account_->account_id->Clone(), mojom::kPolkadotTestnet,
      std::move(extrinsic), block_num, mortality_period);
  ASSERT_FALSE(task);
}

}  // namespace brave_wallet
