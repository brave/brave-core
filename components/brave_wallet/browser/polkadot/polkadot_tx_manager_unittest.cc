/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_manager.h"

#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_forward.h"
#include "base/scoped_observation.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/tx_storage.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48";

inline constexpr const char kBobSS58[] =
    "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3";

inline constexpr char kExpectedExtrinsic[] =
    R"(35028400d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa951601ccbd8179630a0a68e46a81828600655c09c3564e8406d120c18da4cb4460ee10ba8900001c7eb26ee76cac82401d2afa09ae429fdaa4ead2540164cbc98d888755014400000403008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a484913)";

inline constexpr char kExpectedTransferAllExtrinsic[] =
    R"(31028400d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa951601841b7a3756f5c852d6721872cf6abfd00e306018320d378e84b6693ddcd6816f099400e838d8d43e93d93c1d1c585ca4567ad89a43fc79998180dd8da87e3c8955014400000404008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4800)";

class MockTxStateManagerObserver : public TxStateManager::Observer {
 public:
  explicit MockTxStateManagerObserver(TxStateManager& tx_state_manager) {
    observation_.Observe(&tx_state_manager);
  }

  MOCK_METHOD1(OnTransactionStatusChanged, void(mojom::TransactionInfoPtr));
  MOCK_METHOD1(OnNewUnapprovedTx, void(mojom::TransactionInfoPtr));

 private:
  base::ScopedObservation<TxStateManager, TxStateManager::Observer>
      observation_{this};
};

void SetUpMockRpcForFoundExtrinsic(PolkadotMockRpc* polkadot_mock_rpc,
                                   bool was_successful,
                                   bool use_invalid_events) {
  const uint32_t block_num = 29385557u;

  {
    // Pretend that our extrinsic was included in the very first block.
    base::flat_map<uint32_t, std::string> block_hash_map;
    block_hash_map.emplace(
        block_num,
        "0x411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2");

    polkadot_mock_rpc->SetBlockHashMap(std::move(block_hash_map));
  }

  {
    PolkadotBlock block;

    ASSERT_TRUE(base::HexStringToSpan(
        "f6f199e59c1362237dd801d2748274a8ffff0416677b5e2c9bf01d5c7114c759",
        block.header.parent_hash));

    block.header.block_number = block_num;
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

    block.extrinsics = {R"(0x1234)", R"(0xabcd)",
                        base::StrCat({"0x", kExpectedExtrinsic}),
                        base::StrCat({"0x", kExpectedTransferAllExtrinsic})};

    base::flat_map<std::string, PolkadotBlock> block_map;
    block_map.emplace(
        "411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2",
        std::move(block));

    polkadot_mock_rpc->SetBlockMap(std::move(block_map));
  }

  base::flat_map<std::string, std::string> events_map;
  if (was_successful && !use_invalid_events) {
    events_map.emplace(
        "24eb72acd6d597132debd29d2212abf4f635fcce5d4490fe72137b88ae6f42b1",
        R"(0b080000000e000000000001000000000007d41643ad0b997002000000020000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b51300000000000000000000000000020000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95168eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a866140000000000000000000000000000000200000004076d6f646c6461702f7361746c0000000000000000000000000000000000000000dc8df1b51300000000000000000000000000020000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b5130000000000000000000000000000000000000000000000000000000000020000000000823798916da8000000)"
        R"(02000000030000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95162a6502a40300000000000000000000000000030000000004d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95160000030000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa951652707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477ce8e50ea520040000000000000000000000000300000004076d6f646c6461702f7361746c00000000000000000000000000000000000000002a6502a40300000000000000000000000000030000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95162a6502a403000000000000000000000000000000000000000000000000000000000003000000000022c15a916da8000000)");

  } else if (!use_invalid_events) {
    events_map.emplace(
        "24eb72acd6d597132debd29d2212abf4f635fcce5d4490fe72137b88ae6f42b1",
        R"(0b080000000e000000000001000000000007d41643ad0b997002000000020000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b51300000000000000000000000000020000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95168eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a866140000000000000000000000000000000200000004076d6f646c6461702f7361746c0000000000000000000000000000000000000000dc8df1b51300000000000000000000000000020000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b5130000000000000000000000000000000000000000000000000000000000020000000001823798916da8000000)"  //
        R"(02000000030000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95162a6502a40300000000000000000000000000030000000004d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95160000030000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa951652707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477ce8e50ea520040000000000000000000000000300000004076d6f646c6461702f7361746c00000000000000000000000000000000000000002a6502a40300000000000000000000000000030000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95162a6502a403000000000000000000000000000000000000000000000000000000000003000000000022c15a916da8000000)");
  } else {
    // d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516
    events_map.emplace(
        "24eb72acd6d597132debd29d2212abf4f635fcce5d4490fe72137b88ae6f42b1",
        R"(0b080000000e000000000001000000000007d41643ad0b997002000000020000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc8df1b51300000000000000000000000000020000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95168eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48a866140000000000000000000000000000000200000004076d6f646c6461702f7361746c0000000000000000000000000000000000000000dc8df1b51300000000000000000000000000020000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516dc9df1b5130000000000000000000000000000000000000000000000000000000000020000000000823798916da8000000)"
        R"(02000000030000000408d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95162a6502a40300000000000000000000000000030000000004d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95160000030000000402d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa951652707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477ce8e50ea520040000000000000000000000000300000004076d6f646c6461702f7361746c00000000000000000000000000000000000000002a6502a40300000000000000000000000000030000001a00d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa95162a6502a403000000000000000000000000000000000000000000000000000000000003000000000022c15a916da8000000)");
  }
  polkadot_mock_rpc->SetEventsMap(std::move(events_map));
}

}  // namespace

class PolkadotTxManagerUnitTest : public testing::Test {
 public:
  PolkadotTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletPolkadotFeature);

    RegisterProfilePrefs(profile_prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefsForMigration(profile_prefs_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&profile_prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        &profile_prefs_, &local_state_);
    keyring_service_ = std::make_unique<KeyringService>(
        json_rpc_service_.get(), &profile_prefs_, &local_state_);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    polkadot_mock_rpc_ = std::make_unique<PolkadotMockRpc>(
        &url_loader_factory_, network_manager_.get());

    polkadot_wallet_service_ = std::make_unique<PolkadotWalletService>(
        *keyring_service_, *network_manager_, profile_prefs_,
        url_loader_factory_.GetSafeWeakWrapper());

    auto tx_storage = CreateTxStorageForTest(temp_dir_.GetPath());
    tx_storage_ptr_ = tx_storage.get();
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), nullptr, nullptr, nullptr,
        polkadot_wallet_service_.get(), *keyring_service_, &profile_prefs_,
        std::move(tx_storage));

    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateImpl>(*keyring_service_);

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);

    polkadot_mainnet_account_ = keyring_service_->AddAccountSync(
        mojom::CoinType::DOT, mojom::KeyringId::kPolkadotMainnet,
        "mainnet_account");
    polkadot_testnet_account_ = keyring_service_->AddAccountSync(
        mojom::CoinType::DOT, mojom::KeyringId::kPolkadotTestnet,
        "testnet_account");

    UnlockWallet();
  }

  void TearDown() override { tx_storage_ptr_ = nullptr; }

  PolkadotTxManager* GetPolkadotTxManager() {
    return tx_service_->GetPolkadotTxManager();
  }

  PolkadotTxStateManager* GetPolkadotTxStateManager() {
    return &GetPolkadotTxManager()->GetPolkadotTxStateManager();
  }

  PolkadotBlockTracker* GetPolkadotBlockTracker() {
    return &GetPolkadotTxManager()->GetPolkadotBlockTracker();
  }

  void UnlockWallet() {
    keyring_service_->Unlock(
        kTestWalletPassword,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool unlocked) {
              EXPECT_TRUE(unlocked);
              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();

    keyring_service_->polkadot_mainnet_keyring_->SetSignatureRngForTesting();
    keyring_service_->polkadot_testnet_keyring_->SetSignatureRngForTesting();
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  std::string SetUpUnapprovedTx(const std::string& chain_id) {
    auto pubkey = base::HexEncodeLower(
        keyring_service_
            ->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
            .value());

    EXPECT_EQ(
        pubkey,
        "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_testnet_account_->account_id->Clone(), kBob,
        mojom::uint128::New(0, 1234), false, nullptr);

    base::test::TestFuture<bool, const std::string&, const std::string&>
        unapproved_future;

    GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params), unapproved_future.GetCallback());

    auto [success, tx_meta_id, err_str] = unapproved_future.Take();
    EXPECT_TRUE(success);
    EXPECT_FALSE(tx_meta_id.empty());
    EXPECT_EQ(err_str, "");

    return tx_meta_id;
  }

  void ExpectSubmittedTxState(const std::string& tx_meta_id) {
    auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
    ASSERT_TRUE(polkadot_tx);

    EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Submitted);
    EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
    EXPECT_EQ(polkadot_tx->tx()->amount(), uint128_t{1234});
    EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{15937408476ull});
    EXPECT_EQ(polkadot_tx->tx()->transfer_all(), false);
    EXPECT_EQ(polkadot_tx->tx()->extrinsic_metadata()->block_num(), 29385557u);
    EXPECT_EQ(base::HexEncodeLower(
                  polkadot_tx->tx()->extrinsic_metadata()->extrinsic()),
              kExpectedExtrinsic);
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;
  mojom::AccountInfoPtr polkadot_mainnet_account_;
  mojom::AccountInfoPtr polkadot_testnet_account_;

  network::TestURLLoaderFactory url_loader_factory_;

  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  base::ScopedTempDir temp_dir_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<PolkadotMockRpc> polkadot_mock_rpc_;
  std::unique_ptr<PolkadotWalletService> polkadot_wallet_service_;
  std::unique_ptr<TxService> tx_service_;
  std::unique_ptr<AccountResolverDelegateImpl> account_resolver_delegate_;
  raw_ptr<TxStorage> tx_storage_ptr_ = nullptr;
};

TEST_F(PolkadotTxManagerUnitTest, GetCoinType) {
  EXPECT_EQ(GetPolkadotTxManager()->GetCoinType(), mojom::CoinType::DOT);
}

TEST_F(PolkadotTxManagerUnitTest, AddUnapprovedPolkadotTransaction) {
  auto sender_pubkey =
      keyring_service_->GetPolkadotPubKey(polkadot_mainnet_account_->account_id)
          .value();

  polkadot_mock_rpc_->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  {
    // Normal happy path flow of well-formatted data into an accepted unapproved
    // transaction committed to storage.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), kBob,
        mojom::uint128::New(0, 1234), false, nullptr);

    base::test::TestFuture<bool, const std::string&, const std::string&>
        unapproved_future;
    GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params), unapproved_future.GetCallback());
    auto [success, tx_meta_id, err_str] = unapproved_future.Take();

    EXPECT_TRUE(success);
    EXPECT_FALSE(tx_meta_id.empty());
    EXPECT_EQ(err_str, "");

    const auto& txs = tx_storage_ptr_->GetTxs();
    const auto* tx = txs.FindDict(tx_meta_id);
    EXPECT_TRUE(tx);

    const auto* polkadot_tx = tx->FindDict("tx");

    EXPECT_EQ(
        *polkadot_tx->FindString("recipient"),
        R"(8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48)");
    EXPECT_EQ(*polkadot_tx->FindString("amount"),
              "d2040000000000000000000000000000");
    EXPECT_EQ(*polkadot_tx->FindString("fee"),
              "dc8df1b5030000000000000000000000");
    EXPECT_EQ(*polkadot_tx->FindBool("transfer_all"), false);
    EXPECT_EQ(polkadot_tx->FindInt("ss58_prefix"), std::nullopt);

    uint128_t fee = 0;
    EXPECT_TRUE(base::HexStringToSpan(*polkadot_tx->FindString("fee"),
                                      base::byte_span_from_ref(fee)));

    EXPECT_EQ(fee, 15937408476ull);
  }

  {
    // Send a u128::MAX.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), kBobSS58,
        mojom::uint128::New(0xffffffffffffffff, 0xffffffffffffffff), false,
        nullptr);

    base::test::TestFuture<bool, const std::string&, const std::string&>
        unapproved_future;
    GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params), unapproved_future.GetCallback());
    auto [success, tx_meta_id, err_str] = unapproved_future.Take();

    EXPECT_TRUE(success);
    EXPECT_FALSE(tx_meta_id.empty());
    EXPECT_EQ(err_str, "");

    const auto& txs = tx_storage_ptr_->GetTxs();
    const auto* tx = txs.FindDict(tx_meta_id);
    EXPECT_TRUE(tx);

    const auto* polkadot_tx = tx->FindDict("tx");

    EXPECT_EQ(
        *polkadot_tx->FindString("recipient"),
        R"(8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48)");
    EXPECT_EQ(*polkadot_tx->FindString("amount"),
              "ffffffffffffffffffffffffffffffff");
    EXPECT_EQ(*polkadot_tx->FindString("fee"),
              "dc8df1b5030000000000000000000000");
    EXPECT_EQ(*polkadot_tx->FindBool("transfer_all"), false);
    EXPECT_EQ(*polkadot_tx->FindInt("ss58_prefix"), 0);

    uint128_t fee = 0;
    EXPECT_TRUE(base::HexStringToSpan(*polkadot_tx->FindString("fee"),
                                      base::byte_span_from_ref(fee)));

    EXPECT_EQ(fee, 15937408476ull);
  }

  {
    // Provide an invalid destination address to the backend.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), "0x1234",
        mojom::uint128::New(0, 1234), false, nullptr);

    base::test::TestFuture<bool, const std::string&, const std::string&>
        unapproved_future;
    GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params), unapproved_future.GetCallback());
    auto [success, tx_meta_id, err_str] = unapproved_future.Take();

    EXPECT_FALSE(success);
    EXPECT_TRUE(tx_meta_id.empty());
    EXPECT_EQ(err_str, WalletInternalErrorMessage());
  }

  {
    // Provide an incompatible ss58-based address.

    std::string chain_id = mojom::kPolkadotMainnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(),
        "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty",
        mojom::uint128::New(0, 1234), false, nullptr);

    base::test::TestFuture<bool, const std::string&, const std::string&>
        unapproved_future;
    GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params), unapproved_future.GetCallback());
    auto [success, tx_meta_id, err_str] = unapproved_future.Take();

    EXPECT_FALSE(success);
    EXPECT_TRUE(tx_meta_id.empty());
    EXPECT_EQ(err_str, WalletInternalErrorMessage());
  }

  {
    // Provide an invalid chain_id to the backend (i.e. not Polkadot).

    std::string chain_id = mojom::kZCashTestnet;

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, polkadot_mainnet_account_->account_id->Clone(), kBob,
        mojom::uint128::New(0, 1234), false, nullptr);

    base::test::TestFuture<bool, const std::string&, const std::string&>
        unapproved_future;
    GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params), unapproved_future.GetCallback());
    auto [success, tx_meta_id, err_str] = unapproved_future.Take();

    EXPECT_FALSE(success);
    EXPECT_TRUE(tx_meta_id.empty());
    EXPECT_EQ(err_str, WalletInternalErrorMessage());
  }

  {
    // Provide an invalid account id (test account resolution failure).

    std::string chain_id = mojom::kPolkadotMainnet;

    auto account_id = polkadot_mainnet_account_->account_id->Clone();
    account_id->address = "invalid_address";

    auto transaction_params = mojom::NewPolkadotTransactionParams::New(
        chain_id, std::move(account_id), kBob, mojom::uint128::New(0, 1234),
        false, nullptr);

    base::test::TestFuture<bool, const std::string&, const std::string&>
        unapproved_future;
    tx_service_->AddUnapprovedPolkadotTransaction(
        std::move(transaction_params), unapproved_future.GetCallback());
    auto [success, tx_meta_id, err_str] = unapproved_future.Take();

    EXPECT_FALSE(success);
    EXPECT_TRUE(tx_meta_id.empty());
    EXPECT_EQ(err_str,
              l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_FROM_EMPTY));
  }
}

TEST_F(PolkadotTxManagerUnitTest,
       AddUnapprovedPolkadotTransaction_InvalidChainData) {
  // Test the transaction manager when the remote RPC nodes have given us
  // invalid chain data or we've failed the network request (we should be
  // storing a `base::unexpected` in both of these cases).

  polkadot_mock_rpc_->UseInvalidChainMetadata();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotMainnet;

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      chain_id, polkadot_mainnet_account_->account_id.Clone(), kBob,
      mojom::uint128::New(0, 1234), false, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;
  GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());
  auto [success, tx_meta_id, err_str] = unapproved_future.Take();

  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(err_str, WalletInternalErrorMessage());
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_Confirmed) {
  // Prove that our ideal happy flow works for approving transactions.

  SetUpMockRpcForFoundExtrinsic(polkadot_mock_rpc_.get(), true, false);

  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto tx_meta_id = SetUpUnapprovedTx(chain_id);

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success2, error, msg] = approved_future.Take();
  EXPECT_TRUE(success2);

  const auto& txs = tx_storage_ptr_->GetTxs();
  const auto* tx = txs.FindDict(tx_meta_id);
  ASSERT_TRUE(tx);

  ExpectSubmittedTxState(tx_meta_id);

  MockTxStateManagerObserver observer(*GetPolkadotTxStateManager());
  EXPECT_CALL(observer, OnTransactionStatusChanged(testing::_))
      .Times(1)
      .WillOnce(base::test::RunOnceClosure(task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();

  {
    auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
    ASSERT_TRUE(polkadot_tx);

    EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Confirmed);
    EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
    EXPECT_EQ(polkadot_tx->tx()->amount(), uint128_t{1234});
    // Note, our events blob manually changes the fee for the sake of testing.
    EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{84656885212ull});
    EXPECT_EQ(polkadot_tx->tx()->transfer_all(), false);
    EXPECT_EQ(polkadot_tx->tx()->extrinsic_metadata()->block_num(), 29385557u);
    EXPECT_EQ(base::HexEncodeLower(
                  polkadot_tx->tx()->extrinsic_metadata()->extrinsic()),
              kExpectedExtrinsic);
  }

  ASSERT_TRUE(GetPolkadotBlockTracker()->IsRunning(mojom::kPolkadotTestnet));
  GetPolkadotTxManager()->UpdatePendingTransactions(chain_id);
  EXPECT_FALSE(GetPolkadotBlockTracker()->IsRunning(mojom::kPolkadotTestnet));
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_Failed) {
  // Test that we track when an extrinsic is included in a finalized block but
  // it was rejected by the chain.

  SetUpMockRpcForFoundExtrinsic(polkadot_mock_rpc_.get(), false, false);
  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto tx_meta_id = SetUpUnapprovedTx(chain_id);

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success2, error, msg] = approved_future.Take();
  EXPECT_TRUE(success2);

  const auto& txs = tx_storage_ptr_->GetTxs();
  const auto* tx = txs.FindDict(tx_meta_id);
  ASSERT_TRUE(tx);

  ExpectSubmittedTxState(tx_meta_id);

  MockTxStateManagerObserver observer(*GetPolkadotTxStateManager());
  EXPECT_CALL(observer, OnTransactionStatusChanged(testing::_))
      .Times(1)
      .WillOnce(base::test::RunOnceClosure(task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();

  {
    auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
    ASSERT_TRUE(polkadot_tx);

    EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Error);
    EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
    EXPECT_EQ(polkadot_tx->tx()->amount(), uint128_t{1234});
    // Note, our events blob manually changes the fee for the sake of testing.
    EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{84656885212ull});
    EXPECT_EQ(polkadot_tx->tx()->transfer_all(), false);
    EXPECT_EQ(polkadot_tx->tx()->extrinsic_metadata()->block_num(), 29385557u);
    EXPECT_EQ(base::HexEncodeLower(
                  polkadot_tx->tx()->extrinsic_metadata()->extrinsic()),
              kExpectedExtrinsic);
  }
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_InvalidResponse) {
  // Test the case where we've found an extrinsic but the events was slightly
  // corrupted, i.e. fees didn't match between withdrawal and what was cashed by
  // the TransactionPayment pallet.

  SetUpMockRpcForFoundExtrinsic(polkadot_mock_rpc_.get(), false, true);
  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto tx_meta_id = SetUpUnapprovedTx(chain_id);

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success2, error, msg] = approved_future.Take();
  EXPECT_TRUE(success2);

  const auto& txs = tx_storage_ptr_->GetTxs();
  const auto* tx = txs.FindDict(tx_meta_id);
  ASSERT_TRUE(tx);

  ExpectSubmittedTxState(tx_meta_id);

  // We don't send any updates for this scenario.
  MockTxStateManagerObserver observer(*GetPolkadotTxStateManager());
  EXPECT_CALL(observer, OnTransactionStatusChanged(testing::_)).Times(0);

  ExpectSubmittedTxState(tx_meta_id);
  testing::Mock::VerifyAndClearExpectations(&observer);
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_NotFound) {
  // Test that we store when an extrinsic was dropped from the mempool.

  const uint32_t block_num = 29385557u;

  {
    base::flat_map<uint32_t, std::string> block_hash_map;

    for (uint32_t i = 0; i < 64; ++i) {
      block_hash_map.emplace(
          block_num + i,
          "0x411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2");
    }

    polkadot_mock_rpc_->SetBlockHashMap(std::move(block_hash_map));
  }

  {
    base::flat_map<std::string, PolkadotBlock> block_map;
    block_map.emplace(
        "411f460c170a3cda43f42036999a74ea4ae960121cf59fc421a9b4820beadce2",
        PolkadotBlock{});

    polkadot_mock_rpc_->SetBlockMap(std::move(block_map));
  }

  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      chain_id, polkadot_testnet_account_->account_id->Clone(), kBob,
      mojom::uint128::New(0, 1234), false, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;

  GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());

  auto [success, tx_meta_id, err_str] = unapproved_future.Take();
  EXPECT_TRUE(success);
  EXPECT_FALSE(tx_meta_id.empty());
  EXPECT_EQ(err_str, "");

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success2, error, msg] = approved_future.Take();
  EXPECT_TRUE(success2);

  const auto& txs = tx_storage_ptr_->GetTxs();
  const auto* tx = txs.FindDict(tx_meta_id);
  ASSERT_TRUE(tx);

  {
    auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
    ASSERT_TRUE(polkadot_tx);

    EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Submitted);
    EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
    EXPECT_EQ(polkadot_tx->tx()->amount(), uint128_t{1234});
    EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{15937408476ull});
    EXPECT_EQ(polkadot_tx->tx()->transfer_all(), false);
    EXPECT_EQ(polkadot_tx->tx()->extrinsic_metadata()->block_num(), 29385557u);
    EXPECT_EQ(base::HexEncodeLower(
                  polkadot_tx->tx()->extrinsic_metadata()->extrinsic()),
              kExpectedExtrinsic);
  }

  // Advance the finalized header by 128 blocks from the original offset.
  polkadot_mock_rpc_->SetFinalizedBlockHeader(R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c063d5",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  MockTxStateManagerObserver observer(*GetPolkadotTxStateManager());
  EXPECT_CALL(observer, OnTransactionStatusChanged(testing::_))
      .Times(1)
      .WillOnce(base::test::RunOnceClosure(task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();

  {
    auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
    ASSERT_TRUE(polkadot_tx);

    EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Dropped);
    EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
    EXPECT_EQ(polkadot_tx->tx()->amount(), uint128_t{1234});
    // Because the transaction was dropped from the mempool, no fee should've
    // been paid by the sender. Fees are only incurred when an extrinsic is
    // finalized into a block.
    EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{0ull});
    EXPECT_EQ(polkadot_tx->tx()->transfer_all(), false);
    EXPECT_EQ(polkadot_tx->tx()->extrinsic_metadata()->block_num(), 29385557u);
    EXPECT_EQ(base::HexEncodeLower(
                  polkadot_tx->tx()->extrinsic_metadata()->extrinsic()),
              kExpectedExtrinsic);
  }
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_TransferAll) {
  // Prove that our ideal happy flow works for approving transactions, this time
  // for a transfer_all call.

  SetUpMockRpcForFoundExtrinsic(polkadot_mock_rpc_.get(), true, false);
  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  uint128_t amount = uint128_t{19079223034968ull};

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      chain_id, polkadot_testnet_account_->account_id->Clone(), kBob,
      Uint128ToMojom(amount), true, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;

  GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());

  auto [success, tx_meta_id, err_str] = unapproved_future.Take();
  EXPECT_TRUE(success);
  EXPECT_FALSE(tx_meta_id.empty());
  EXPECT_EQ(err_str, "");

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success2, error, msg] = approved_future.Take();
  EXPECT_TRUE(success2);

  const auto& txs = tx_storage_ptr_->GetTxs();
  const auto* tx = txs.FindDict(tx_meta_id);
  ASSERT_TRUE(tx);

  {
    auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
    ASSERT_TRUE(polkadot_tx);

    EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Submitted);
    EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
    EXPECT_EQ(polkadot_tx->tx()->amount(), amount - uint128_t{15937408476ull});
    EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{15937408476ull});
    EXPECT_EQ(polkadot_tx->tx()->transfer_all(), true);
    EXPECT_EQ(polkadot_tx->tx()->extrinsic_metadata()->block_num(), 29385557u);
  }

  MockTxStateManagerObserver observer(*GetPolkadotTxStateManager());
  EXPECT_CALL(observer, OnTransactionStatusChanged(testing::_))
      .Times(1)
      .WillOnce(base::test::RunOnceClosure(task_environment_.QuitClosure()));

  task_environment_.RunUntilQuit();

  {
    const uint128_t actual_fee = 15636522282ull;

    auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
    ASSERT_TRUE(polkadot_tx);

    EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Confirmed);
    EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
    EXPECT_EQ(polkadot_tx->tx()->amount(), amount - actual_fee);
    EXPECT_EQ(polkadot_tx->tx()->fee(), actual_fee);
    EXPECT_EQ(polkadot_tx->tx()->transfer_all(), true);
    EXPECT_EQ(polkadot_tx->tx()->extrinsic_metadata()->block_num(), 29385557u);
  }
}

TEST_F(PolkadotTxManagerUnitTest, TransferAll_InsufficientAmount) {
  // The front-end should prevent this but just in case, we explicitly handle
  // the case where a fee exceeds the total amount a user might be sending when
  // they click MAX in the send UI.

  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      chain_id, polkadot_testnet_account_->account_id->Clone(), kBob,
      mojom::uint128::New(0, 1234), true, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;

  GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());

  auto [success, tx_meta_id, err_str] = unapproved_future.Take();
  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(err_str, WalletInsufficientBalanceErrorMessage());
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_NoTransaction) {
  // Test the endpoint when the tx_meta_id can't be found.

  std::string chain_id = mojom::kPolkadotTestnet;
  std::string tx_meta_id = "random_stuff";

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success, error, str] = approved_future.Take();
  EXPECT_FALSE(success);
  EXPECT_EQ(error->get_polkadot_provider_error(),
            mojom::PolkadotProviderError::kInternalError);
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_RejectedExtrinsic) {
  // Prove that we can handle the case where the RPC nodes reject the extrinsic
  // outright without returning a transaction hash.

  polkadot_mock_rpc_->RejectExtrinsicSubmission();
  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      chain_id, polkadot_testnet_account_->account_id->Clone(), kBob,
      mojom::uint128::New(0, 1234), false, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;

  GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());

  auto [success, tx_meta_id, err_str] = unapproved_future.Take();
  EXPECT_TRUE(success);
  EXPECT_FALSE(tx_meta_id.empty());
  EXPECT_EQ(err_str, "");

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success2, error, msg] = approved_future.Take();
  EXPECT_FALSE(success2);

  const auto& txs = tx_storage_ptr_->GetTxs();
  const auto* tx = txs.FindDict(tx_meta_id);
  ASSERT_TRUE(tx);

  auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
  ASSERT_TRUE(polkadot_tx);

  EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Error);
  EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
  EXPECT_EQ(polkadot_tx->tx()->amount(), uint128_t{1234});
  EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{15937408476ull});
  EXPECT_EQ(polkadot_tx->tx()->transfer_all(), false);
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction_NetworkFailure) {
  // Prove that we can handle the case where an intermittent network failure
  // during extrinsic signing fails.

  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  std::string chain_id = mojom::kPolkadotTestnet;

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "d6b2a5cc606ea86342001dd036b301c15a5cba63c413cad5ca0e8f47e6fa9516");

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      chain_id, polkadot_testnet_account_->account_id->Clone(), kBob,
      mojom::uint128::New(0, 1234), false, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;

  GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());

  auto [success, tx_meta_id, err_str] = unapproved_future.Take();
  EXPECT_TRUE(success);
  EXPECT_FALSE(tx_meta_id.empty());
  EXPECT_EQ(err_str, "");

  polkadot_mock_rpc_->RejectAccountInfoRequest();

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;

  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [success2, error, msg] = approved_future.Take();
  EXPECT_FALSE(success2);

  const auto& txs = tx_storage_ptr_->GetTxs();
  const auto* tx = txs.FindDict(tx_meta_id);
  ASSERT_TRUE(tx);

  auto polkadot_tx = GetPolkadotTxManager()->GetPolkadotTx(tx_meta_id);
  ASSERT_TRUE(polkadot_tx);

  EXPECT_EQ(polkadot_tx->status(), mojom::TransactionStatus::Error);
  EXPECT_EQ(polkadot_tx->tx()->recipient().ToString().value(), kBob);
  EXPECT_EQ(polkadot_tx->tx()->amount(), uint128_t{1234});
  EXPECT_EQ(polkadot_tx->tx()->fee(), uint128_t{15937408476ull});
  EXPECT_EQ(polkadot_tx->tx()->transfer_all(), false);
}

TEST_F(PolkadotTxManagerUnitTest, SpeedupOrCancelTransaction) {
  GetPolkadotTxManager()->SpeedupOrCancelTransaction(
      "test_tx_id", false,  // false = speedup, true = cancel
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

TEST_F(PolkadotTxManagerUnitTest, RetryTransaction) {
  GetPolkadotTxManager()->RetryTransaction(
      "test_tx_id",
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

TEST_F(PolkadotTxManagerUnitTest, RestrictedFromAddress) {
  BlockchainRegistry::ScopedRestrictedAddressesForTesting scoped_restricted(
      {base::ToLowerASCII(polkadot_mainnet_account_->address)});

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      mojom::kPolkadotMainnet, polkadot_mainnet_account_->account_id->Clone(),
      kBobSS58, mojom::uint128::New(0, 1234), false, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;

  tx_service_->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());

  const auto& [success, tx_meta_id, err_str] = unapproved_future.Take();

  EXPECT_FALSE(success);
  EXPECT_TRUE(tx_meta_id.empty());
  EXPECT_EQ(err_str, WalletInternalErrorMessage());
}

TEST_F(PolkadotTxManagerUnitTest, UpdatePendingTransactions_BlockTracker) {
  // Test that calling UpdatePendingTransactions() doesn't remove any previously
  // registered block trackers.

  SetUpMockRpcForFoundExtrinsic(polkadot_mock_rpc_.get(), true, false);
  polkadot_mock_rpc_->AddReqResPairs();
  polkadot_mock_rpc_->FinalizeSetup();

  auto transaction_params = mojom::NewPolkadotTransactionParams::New(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id->Clone(),
      kBob, mojom::uint128::New(0, 1234), false, nullptr);

  base::test::TestFuture<bool, const std::string&, const std::string&>
      unapproved_future;
  GetPolkadotTxManager()->AddUnapprovedPolkadotTransaction(
      std::move(transaction_params), unapproved_future.GetCallback());

  auto [add_ok, tx_meta_id, add_err] = unapproved_future.Take();
  ASSERT_TRUE(add_ok);

  base::test::TestFuture<bool, mojom::ProviderErrorUnionPtr, const std::string&>
      approved_future;
  GetPolkadotTxManager()->ApproveTransaction(tx_meta_id,
                                             approved_future.GetCallback());

  auto [approve_ok, error, approve_err] = approved_future.Take();
  ASSERT_TRUE(approve_ok);

  // Approval calls UpdatePendingTransactions(testnet), which starts the
  // tracker.
  ASSERT_TRUE(GetPolkadotBlockTracker()->IsRunning(mojom::kPolkadotTestnet));

  GetPolkadotTxManager()->UpdatePendingTransactions(mojom::kPolkadotMainnet);
  EXPECT_TRUE(GetPolkadotBlockTracker()->IsRunning(mojom::kPolkadotTestnet));
}

}  // namespace brave_wallet
