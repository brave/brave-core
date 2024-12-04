/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override {}

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));

  MOCK_METHOD3(GetTreeState,
               void(const std::string& chain_id,
                    zcash::mojom::BlockIDPtr block,
                    GetTreeStateCallback callback));

  MOCK_METHOD4(GetCompactBlocks,
               void(const std::string& chain_id,
                    uint32_t from,
                    uint32_t to,
                    GetCompactBlocksCallback callback));

  MOCK_METHOD2(GetLatestTreeState,
               void(const std::string& chain_id,
                    GetTreeStateCallback callback));
};

class MockZCashShieldSyncServiceObserver
    : public ZCashShieldSyncService::Observer {
 public:
  MOCK_METHOD1(OnSyncStart, void(const mojom::AccountIdPtr& account_id));

  std::vector<mojom::ZCashShieldSyncStatusPtr>& GetStatuses() {
    return statuses_;
  }

  void OnSyncStatusUpdate(
      const mojom::AccountIdPtr& account_id,
      const mojom::ZCashShieldSyncStatusPtr& status) override {
    statuses_.push_back(status.Clone());
  }

  MOCK_METHOD1(OnSyncStop, void(const mojom::AccountIdPtr& account_id));
  MOCK_METHOD2(OnSyncError,
               void(const mojom::AccountIdPtr& account_id,
                    const std::string& error));

  base::WeakPtr<MockZCashShieldSyncServiceObserver> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  std::vector<mojom::ZCashShieldSyncStatusPtr> statuses_;

  base::WeakPtrFactory<MockZCashShieldSyncServiceObserver> weak_ptr_factory_{
      this};
};

}  // namespace

class MockOrchardBlockScannerProxy
    : public ZCashShieldSyncService::OrchardBlockScannerProxy {
 public:
  using Callback = base::RepeatingCallback<void(
      OrchardTreeState,
      std::vector<zcash::mojom::CompactBlockPtr>,
      base::OnceCallback<void(base::expected<OrchardBlockScanner::Result,
                                             OrchardBlockScanner::ErrorCode>)>
          callback)>;

  explicit MockOrchardBlockScannerProxy(Callback callback)
      : OrchardBlockScannerProxy({}), callback_(callback) {}

  ~MockOrchardBlockScannerProxy() override = default;

  void ScanBlocks(
      OrchardTreeState tree_state,
      std::vector<zcash::mojom::CompactBlockPtr> blocks,
      base::OnceCallback<void(base::expected<OrchardBlockScanner::Result,
                                             OrchardBlockScanner::ErrorCode>)>
          callback) override {
    callback_.Run(std::move(tree_state), std::move(blocks),
                  std::move(callback));
  }

 private:
  Callback callback_;
};

class ZCashShieldSyncServiceTest : public testing::Test {
 public:
  ZCashShieldSyncServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath db_path(
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    sync_state_.emplace(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path.AppendASCII("orchard.db"));

    observer_ = std::make_unique<MockZCashShieldSyncServiceObserver>();

    ResetSyncService();
  }

  void ResetSyncService() {
    auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
    auto account_birthday =
        mojom::ZCashAccountShieldBirthday::New(kNu5BlockUpdate + 100, "hash");
    OrchardFullViewKey fvk;

    sync_service_ = std::make_unique<ZCashShieldSyncService>(
        zcash_rpc_, sync_state_, account_id, account_birthday, fvk,
        observer_->GetWeakPtr());

    // Ensure previous OrchardStorage is destroyed on background thread
    task_environment_.RunUntilIdle();
  }

  ZCashShieldSyncService* sync_service() { return sync_service_.get(); }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  MockZCashShieldSyncServiceObserver* observer() { return observer_.get(); }

  mojom::AccountIdPtr account() { return zcash_account_.Clone(); }

  std::unique_ptr<MockOrchardBlockScannerProxy>
  CreateMockOrchardBlockScannerProxy() {
    return std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
        [](OrchardTreeState tree_state,
           std::vector<zcash::mojom::CompactBlockPtr> blocks,
           base::OnceCallback<void(
               base::expected<OrchardBlockScanner::Result,
                              OrchardBlockScanner::ErrorCode>)> callback) {
          auto account_id = MakeIndexBasedAccountId(
              mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
              mojom::AccountKind::kDerived, 0);
          OrchardBlockScanner::Result result = CreateResultForTesting(
              std::move(tree_state), std::vector<OrchardCommitment>());
          for (const auto& block : blocks) {
            // 3 notes in the blockchain
            if (block->height == kNu5BlockUpdate + 105) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 1));
            } else if (block->height == kNu5BlockUpdate + 205) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 2));
            } else if (block->height == kNu5BlockUpdate + 305) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 3));
            }

            // First 2 notes are spent
            if (block->height == kNu5BlockUpdate + 255) {
              result.found_spends.push_back(OrchardNoteSpend{
                  block->height, GenerateMockNullifier(account_id, 1)});
            } else if (block->height == kNu5BlockUpdate + 265) {
              result.found_spends.push_back(OrchardNoteSpend{
                  block->height, GenerateMockNullifier(account_id, 2)});
            }
          }
          std::move(callback).Run(std::move(result));
        }));
  }

  mojom::AccountIdPtr zcash_account_;
  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<KeyringService> keyring_service_;
  base::SequenceBound<OrchardSyncState> sync_state_;
  testing::NiceMock<MockZCashRPC> zcash_rpc_;
  std::unique_ptr<MockZCashShieldSyncServiceObserver> observer_;
  std::unique_ptr<ZCashShieldSyncService> sync_service_;
  base::test::TaskEnvironment task_environment_;
};

TEST_F(ZCashShieldSyncServiceTest, ScanBlocks) {
  auto mock_block_scanner = CreateMockOrchardBlockScannerProxy();

  sync_service()->SetOrchardBlockScannerProxyForTesting(
      std::move(mock_block_scanner));

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 500u, std::vector<uint8_t>({})));
          }));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            // Valid tree state
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabb", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, uint32_t from, uint32_t to,
             ZCashRpc::GetCompactBlocksCallback callback) {
            std::vector<zcash::mojom::CompactBlockPtr> blocks;
            for (uint32_t i = from; i <= to; i++) {
              // Create empty block for testing
              blocks.push_back(zcash::mojom::CompactBlock::New(
                  0u, i, std::vector<uint8_t>({0xbb, 0xaa}),
                  std::vector<uint8_t>(), 0u, std::vector<uint8_t>(),
                  std::vector<zcash::mojom::CompactTxPtr>(), nullptr));
            }
            std::move(callback).Run(std::move(blocks));
          }));

  {
    sync_service()->StartSyncing();
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_EQ(sync_status->spendable_balance, 30u);
  }

  // Resume scanning
  ResetSyncService();
  task_environment_.RunUntilIdle();

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 1000u, std::vector<uint8_t>({})));
          }));

  sync_service()->SetOrchardBlockScannerProxyForTesting(
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            auto account_id = MakeIndexBasedAccountId(
                mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
                mojom::AccountKind::kDerived, 0);
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>());
            for (const auto& block : blocks) {
              // 3 notes in the blockchain
              if (block->height == kNu5BlockUpdate + 605) {
                result.discovered_notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 5));
              } else if (block->height == kNu5BlockUpdate + 705) {
                result.discovered_notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 6));
              } else if (block->height == kNu5BlockUpdate + 905) {
                result.discovered_notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 7));
              }

              // First 2 notes are spent
              if (block->height == kNu5BlockUpdate + 855) {
                result.found_spends.push_back(OrchardNoteSpend{
                    block->height, GenerateMockNullifier(account_id, 3)});
              }
            }
            std::move(callback).Run(std::move(result));
          })));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            // Valid tree state
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabbcc", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  {
    sync_service()->StartSyncing();
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_EQ(sync_status->spendable_balance, 180u);
  }

  // Chain reorg when chain tip is greater than latest scanned block
  ResetSyncService();

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 1010u, std::vector<uint8_t>({})));
          }));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            // Hash of the latest scanned block
            // Tree state has been changed
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabbccdd", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  sync_service()->SetOrchardBlockScannerProxyForTesting(
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>());
            std::move(callback).Run(std::move(result));
          })));

  {
    sync_service()->StartSyncing();
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_EQ(sync_status->spendable_balance, 140u);
  }

  // Chain reorg when chain tip is less than latest scanned block
  ResetSyncService();

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 950u, std::vector<uint8_t>({})));
          }));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            // Hash of the latest scanned block differs
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabbccddee", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  sync_service()->SetOrchardBlockScannerProxyForTesting(
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            auto account_id = MakeIndexBasedAccountId(
                mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
                mojom::AccountKind::kDerived, 0);
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>());
            for (const auto& block : blocks) {
              // First block is the current chain tip - kChainReorgBlockDelta
              EXPECT_GE(block->height,
                        kNu5BlockUpdate + 950u - kChainReorgBlockDelta);
              // Last block is the current chain tip
              EXPECT_GE(kNu5BlockUpdate + 950u, block->height);

              // Add a new note on height 900
              if (block->height == kNu5BlockUpdate + 900) {
                result.discovered_notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 8));
              }

              // Add a nullifier for previous note
              if (block->height == kNu5BlockUpdate + 905) {
                result.found_spends.push_back(OrchardNoteSpend{
                    block->height, GenerateMockNullifier(account_id, 3)});
              }
            }
            std::move(callback).Run(std::move(result));
          })));

  {
    sync_service()->StartSyncing();
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_EQ(sync_status->spendable_balance, 190u);
  }
}

}  // namespace brave_wallet
