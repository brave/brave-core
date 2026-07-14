/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_complete_transaction_task.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;

namespace brave_wallet {

namespace {

// Mirrors the mocks in zcash_wallet_service_unittest.cc (the only place that
// currently drives ZCashCompleteTransactionTask end-to-end, via
// ZCashWalletService::SignAndPostTransaction) restricted to the RPC calls
// WorkOnTask() actually makes: consensus branch id, chain tip, and the
// per-pool anchor tree state.
class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override {}

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));

  MOCK_METHOD3(GetTreeState,
               void(const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block_id,
                    GetTreeStateCallback callback));

  MOCK_METHOD2(GetLightdInfo,
               void(const std::string& chain_id,
                    GetLightdInfoCallback callback));
};

class MockOrchardSyncState : public OrchardSyncState {
 public:
  explicit MockOrchardSyncState(const base::FilePath& path_to_database)
      : OrchardSyncState(path_to_database) {}
  ~MockOrchardSyncState() override {}

  MOCK_METHOD4(CalculateWitnessForCheckpoint,
               base::expected<std::vector<OrchardInput>, OrchardStorage::Error>(
                   OrchardPool pool,
                   const mojom::AccountIdPtr& account_id,
                   const std::vector<OrchardInput>& notes,
                   uint32_t checkpoint_position));
};

}  // namespace

// Real scaffolding for testing ZCashCompleteTransactionTask — the class that
// signs the transparent part and proves/signs the shielded part(s) of a
// transaction (v5's single orchard pool, or v6's dual legacy-orchard +
// ironwood pools) before it is broadcast.
//
// Unlike ZCashCreateOrchardToOrchardTransactionTask, this task's constructor
// only accepts a base::PassKey<ZCashWalletService> (see
// zcash_complete_transaction_task.h), so it can't be instantiated directly
// from a test. Real (non-stub) tests must drive it indirectly through
// ZCashWalletService::SignAndPostTransaction(), the same way the existing v5
// coverage in zcash_wallet_service_unittest.cc does. Model on that pattern,
// plus zcash_create_orchard_to_orchard_transaction_task_unittest.cc for the
// SetUp() shape, once real Ironwood v3 fixtures are available.
class ZCashCompleteTransactionTaskTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          {{"zcash_shielded_transactions_enabled", "true"},
           {"zcash_ironwood_transaction_enabled", "true"}}},
#if BUILDFLAG(IS_IOS)
         {features::kBraveWalletWebUIFeature, {}}
#endif
        },
        {}  // disabled features
    );

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                    false, base::DoNothing());

    zcash_wallet_service_ = std::make_unique<TestingZCashWalletService>(
        *keyring_service_,
        std::make_unique<testing::NiceMock<MockZCashRPC>>());
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<testing::NiceMock<MockOrchardSyncState>>(
            temp_dir_.GetPath()));

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();
  }

  testing::NiceMock<MockZCashRPC>& mock_zcash_rpc() {
    return static_cast<testing::NiceMock<MockZCashRPC>&>(
        zcash_wallet_service_->zcash_rpc());
  }

  testing::NiceMock<MockOrchardSyncState>& mock_orchard_sync_state() {
    return static_cast<testing::NiceMock<MockOrchardSyncState>&>(
        *zcash_wallet_service_->sync_state_ptr);
  }

  ZCashWalletService& zcash_wallet_service() { return *zcash_wallet_service_; }

  KeyringService& keyring_service() { return *keyring_service_; }

  const mojom::AccountIdPtr& account_id() { return account_id_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

 private:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;

  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TestingZCashWalletService> zcash_wallet_service_;
};

// TODO(ironwood): fill in with real testnet fixtures — Ironwood (v3) commitment
// tree state + spendable V3 notes + fvk/sk to build and sign a real bundle.
// See orchard_bundle_manager_unittest.cc for the deterministic-rng proving
// pattern (OrchardBundleManager::OverrideRandomSeedForTesting /
// create_testing_orchard_bundle), which currently only covers Orchard/V2.
TEST_F(ZCashCompleteTransactionTaskTest, DISABLED_SignsV6DualPoolBundle) {
  // Intended scenario: a v6 ZCashTransaction with both `v6_part().
  // legacy_orchard` (existing orchard pool, real spends/outputs already
  // supported) and `v6_part().ironwood` (new pool, needs real V3 notes)
  // populated with at least one input and one output each, run through
  // ZCashWalletService::SignAndPostTransaction() (which owns and drives
  // ZCashCompleteTransactionTask). Once complete, both
  // `v6_part().legacy_orchard.raw_tx`/`.digest` and
  // `v6_part().ironwood.raw_tx`/`.digest` should be populated, and both
  // bundles should have been authorized using the single shared v6 sighash
  // computed once by ZCashCompleteTransactionTask::SignOrchardBundlesV6()
  // (see zcash_complete_transaction_task.cc).
  ZCashTransaction unsigned_tx;
  unsigned_tx.ConvertToV6();
  unsigned_tx.v6_part().legacy_orchard.anchor_block_height = 10u;
  unsigned_tx.v6_part().ironwood.anchor_block_height = 10u;

  GTEST_SKIP() << "Needs real Ironwood v3 transaction fixtures.";
}

// TODO(ironwood): fill in with real testnet fixtures — Ironwood (v3) commitment
// tree state + spendable V3 notes + fvk/sk to build and sign a real bundle.
// See orchard_bundle_manager_unittest.cc for the deterministic-rng proving
// pattern (OrchardBundleManager::OverrideRandomSeedForTesting /
// create_testing_orchard_bundle), which currently only covers Orchard/V2.
TEST_F(ZCashCompleteTransactionTaskTest,
       DISABLED_UsesIronwoodTreeForIronwoodAnchor) {
  // Intended scenario (WI-2 anchor-tree fix): BuildOrchardBundleV6(pool) in
  // zcash_complete_transaction_task.cc now branches on `pool ==
  // OrchardPool::kIronwood` to read `state.tree_state.value()->ironwoodTree`
  // instead of `orchardTree` (both fields on
  // zcash::mojom::TreeState, see zcash_decoder.mojom) before calling
  // OrchardBundleManager::Create(). A real test would mock GetTreeState() to
  // return a TreeState with distinguishable orchardTree/ironwoodTree hex
  // payloads, request both pools' anchors via a v6 transaction with both
  // legacy_orchard and ironwood populated, and assert the ironwood pool's
  // bundle is built from the ironwoodTree bytes while the legacy_orchard
  // pool's bundle is built from the orchardTree bytes. Exercising this
  // meaningfully (rather than just observing which hex string was read)
  // needs a real Ironwood commitment tree fixture that GetTreeStateV6() /
  // BuildOrchardBundleV6() can actually build a valid unauthorized bundle
  // from.
  GTEST_SKIP() << "Needs real Ironwood v3 transaction fixtures.";
}

// TODO(ironwood): fill in with real testnet fixtures — Ironwood (v3) commitment
// tree state + spendable V3 notes + fvk/sk to build and sign a real bundle.
// See orchard_bundle_manager_unittest.cc for the deterministic-rng proving
// pattern (OrchardBundleManager::OverrideRandomSeedForTesting /
// create_testing_orchard_bundle), which currently only covers Orchard/V2.
TEST_F(ZCashCompleteTransactionTaskTest,
       DISABLED_SharedSighashCoversBothPools) {
  // Intended scenario: per ZIP-246, a v6 transaction has a single signature
  // digest covering both shielded pools. ZCashCompleteTransactionTask
  // computes this once, in SignOrchardBundlesV6() (see
  // zcash_complete_transaction_task.cc), via
  // `ZCashSerializer::CalculateSignatureDigest(transaction_, std::nullopt)`,
  // caches it in `v6_sighash_`, and reuses the same value for both pools'
  // `OrchardBundleManager::ApplySignature()` calls — it is not recomputed
  // per pool. A real test would build a v6 transaction with both pools
  // populated, let ZCashCompleteTransactionTask build both pools' unauthorized
  // digests (populating `legacy_orchard.digest` and `ironwood.digest`), then
  // assert that the sighash passed to both pools' ApplySignature() calls is
  // identical and matches
  // ZCashSerializer::CalculateSignatureDigest(transaction_, std::nullopt)
  // computed once both digests are set.
  GTEST_SKIP() << "Needs real Ironwood v3 transaction fixtures.";
}

}  // namespace brave_wallet
