/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task_v6.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using testing::_;

namespace brave_wallet {

namespace {

constexpr char kConsensusBranchId[] = "37a5165b";
constexpr uint32_t kAnchorHeight = 3446634u;
constexpr uint32_t kChainTipHeight = 3446639u;

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

  MOCK_METHOD2(GetLightdInfo,
               void(const std::string& chain_id,
                    GetLightdInfoCallback callback));

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));

  MOCK_METHOD3(GetTreeState,
               void(const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block_id,
                    GetTreeStateCallback callback));
};

class MockOrchardSyncState : public OrchardSyncState {
 public:
  explicit MockOrchardSyncState(const base::FilePath& path_to_database)
      : OrchardSyncState(path_to_database) {}
  ~MockOrchardSyncState() override = default;

  MOCK_METHOD4(CalculateWitnessForCheckpoint,
               base::expected<std::vector<OrchardInput>, OrchardStorage::Error>(
                   OrchardPool pool,
                   const mojom::AccountIdPtr& account_id,
                   const std::vector<OrchardInput>& notes,
                   uint32_t checkpoint_position));
};

OrchardOutput MakeOrchardOutput(uint64_t amount) {
  OrchardOutput output;
  output.value = amount;
  output.addr = {212, 113, 78,  231, 97,  209, 174, 130, 59,  105, 114,
                 21,  46,  32,  149, 127, 239, 163, 246, 227, 18,  158,
                 164, 223, 176, 169, 233, 135, 3,   166, 61,  171, 146,
                 149, 137, 214, 220, 81,  201, 112, 249, 53,  179};
  return output;
}

ZCashTransaction MakeShieldedV6Transaction(
    const mojom::AccountIdPtr& account_id) {
  ZCashTransaction tx;
  tx.init_v6_part();
  tx.set_fee(5000u);

  auto& orchard = tx.v6_part().legacy_orchard;
  orchard.anchor_block_height = kAnchorHeight;
  OrchardInput input;
  input.note = GenerateMockOrchardNote(account_id, kAnchorHeight, 1, 50000u);
  orchard.inputs.push_back(std::move(input));
  orchard.outputs.push_back(MakeOrchardOutput(45000u));
  return tx;
}

zcash::mojom::TreeStatePtr MakeTreeState(const std::string& orchard_tree) {
  return zcash::mojom::TreeState::New(
      "main", kAnchorHeight,
      "00000000006f3ecd55d028513d2e8438520afba943b9b08c7a2f1a8d60a09b5f", 0u,
      "" /* sapling tree */, orchard_tree, "" /* ironwood tree */);
}

}  // namespace

class ZCashCompleteTransactionTaskV6Test : public testing::Test {
 public:
  void SetUp() override { InitializeFixture(/*shielded_enabled=*/true); }

 protected:
  void InitializeFixture(bool shielded_enabled) {
    InitFeatureList(shielded_enabled);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                    false, base::DoNothing());

    zcash_wallet_service_ = std::make_unique<TestingZCashWalletService>(
        *keyring_service_, std::make_unique<testing::NiceMock<MockZCashRPC>>());
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<MockOrchardSyncState>(temp_dir_.GetPath()));

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();

    SetupSuccessfulRpcDefaults();
    SetupSuccessfulWitnessDefault();
  }

  void InitFeatureList(bool shielded_enabled) {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          {{"zcash_shielded_transactions_enabled",
            shielded_enabled ? "true" : "false"}}},
#if BUILDFLAG(IS_IOS)
         {features::kBraveWalletWebUIFeature, {}}
#endif
        },
        {});
  }

  void SetupSuccessfulRpcDefaults() {
    ON_CALL(mock_zcash_rpc(), GetLightdInfo(_, _))
        .WillByDefault([](const std::string& chain_id,
                          ZCashRpc::GetLightdInfoCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          std::move(callback).Run(
              zcash::mojom::LightdInfo::New(kConsensusBranchId));
        });

    ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
        .WillByDefault([](const std::string& chain_id,
                          ZCashRpc::GetLatestBlockCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          std::move(callback).Run(zcash::mojom::BlockID::New(
              kChainTipHeight, std::vector<uint8_t>({})));
        });

    ON_CALL(mock_zcash_rpc(), GetTreeState(_, _, _))
        .WillByDefault([](const std::string& chain_id,
                          zcash::mojom::BlockIDPtr block_id,
                          ZCashRpc::GetTreeStateCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          EXPECT_EQ(block_id->height, kAnchorHeight);
          std::move(callback).Run(MakeTreeState("00"));
        });
  }

  void SetupSuccessfulWitnessDefault() {
    ON_CALL(mock_orchard_sync_state(),
            CalculateWitnessForCheckpoint(_, _, _, _))
        .WillByDefault(
            [](OrchardPool pool, const mojom::AccountIdPtr& account_id,
               const std::vector<OrchardInput>& notes,
               uint32_t checkpoint_position) { return base::ok(notes); });
  }

  base::expected<ZCashTransaction, std::string> RunTask(
      const ZCashTransaction& transaction) {
    return RunTask(transaction, action_context());
  }

  base::expected<ZCashTransaction, std::string> RunTask(
      const ZCashTransaction& transaction,
      ZCashActionContext context) {
    base::test::TestFuture<base::expected<ZCashTransaction, std::string>>
        result_future;

    auto task = std::make_unique<ZCashCompleteTransactionTaskV6>(
        pass_key(), zcash_wallet_service(), std::move(context),
        keyring_service(), transaction);
    task->Start(result_future.GetCallback());

    return result_future.Get();
  }

  base::PassKey<ZCashCompleteTransactionTaskV6Test> pass_key() {
    return base::PassKey<ZCashCompleteTransactionTaskV6Test>();
  }

  std::string InternalErrorString() const {
    return l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
  }

  TestingZCashWalletService& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  KeyringService& keyring_service() { return *keyring_service_; }

  MockZCashRPC& mock_zcash_rpc() {
    return static_cast<MockZCashRPC&>(zcash_wallet_service().zcash_rpc());
  }

  MockOrchardSyncState& mock_orchard_sync_state() {
    return static_cast<MockOrchardSyncState&>(
        *zcash_wallet_service_->sync_state_ptr);
  }

  ZCashActionContext action_context() {
    return zcash_wallet_service().CreateActionContext(account_id());
  }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  base::test::ScopedFeatureList feature_list_;

 private:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TestingZCashWalletService> zcash_wallet_service_;
};

class ZCashCompleteTransactionTaskV6NoShieldedTest
    : public ZCashCompleteTransactionTaskV6Test {
 public:
  void SetUp() override { InitializeFixture(/*shielded_enabled=*/false); }
};

TEST_F(ZCashCompleteTransactionTaskV6Test, GetLightdInfoError) {
  EXPECT_CALL(mock_zcash_rpc(), GetLightdInfo(_, _))
      .WillOnce([](const std::string& chain_id,
                   ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(
            base::unexpected<std::string>("lightd unavailable"));
      });

  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6Test, InvalidConsensusBranchId) {
  EXPECT_CALL(mock_zcash_rpc(), GetLightdInfo(_, _))
      .WillOnce([](const std::string& chain_id,
                   ZCashRpc::GetLightdInfoCallback callback) {
        std::move(callback).Run(
            zcash::mojom::LightdInfo::New("not-a-valid-hex-branch-id"));
      });

  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6Test, GetLatestBlockError) {
  EXPECT_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillOnce([](const std::string& chain_id,
                   ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            base::unexpected<std::string>("latest block unavailable"));
      });

  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6Test, WitnessCalculateError) {
  EXPECT_CALL(mock_orchard_sync_state(),
              CalculateWitnessForCheckpoint(_, _, _, _))
      .WillOnce([](OrchardPool pool, const mojom::AccountIdPtr& account_id,
                   const std::vector<OrchardInput>& notes,
                   uint32_t checkpoint_position) {
        return base::unexpected(OrchardStorage::Error{
            OrchardStorage::ErrorCode::kConsistencyError, "witness failed"});
      });

  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6Test, GetTreeStateError) {
  EXPECT_CALL(mock_zcash_rpc(), GetTreeState(_, _, _))
      .WillOnce([](const std::string& chain_id,
                   zcash::mojom::BlockIDPtr block_id,
                   ZCashRpc::GetTreeStateCallback callback) {
        std::move(callback).Run(
            base::unexpected<std::string>("tree state unavailable"));
      });

  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6Test, InvalidTreeStateHex) {
  EXPECT_CALL(mock_zcash_rpc(), GetTreeState(_, _, _))
      .WillOnce([](const std::string& chain_id,
                   zcash::mojom::BlockIDPtr block_id,
                   ZCashRpc::GetTreeStateCallback callback) {
        std::move(callback).Run(MakeTreeState("not_valid_hex"));
      });

  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6NoShieldedTest, MissingOrchardKeys) {
  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6Test, OrchardBundleCreationFailed) {
  EXPECT_CALL(mock_zcash_rpc(), GetTreeState(_, _, _))
      .WillOnce([](const std::string& chain_id,
                   zcash::mojom::BlockIDPtr block_id,
                   ZCashRpc::GetTreeStateCallback callback) {
        std::move(callback).Run(MakeTreeState("00"));
      });

  // Inputs returned by the witness mock do not include merkle paths, so bundle
  // creation fails once FVK/SK are resolved.
  auto result = RunTask(MakeShieldedV6Transaction(account_id()));
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), InternalErrorString());
}

TEST_F(ZCashCompleteTransactionTaskV6Test, AnchorNotSelected) {
  auto tx = MakeShieldedV6Transaction(account_id());
  tx.v6_part().legacy_orchard.anchor_block_height.reset();

  auto result = RunTask(tx);
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Anchor not selected");
}

}  // namespace brave_wallet
