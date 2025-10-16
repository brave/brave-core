/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {

class MockZCashWalletService : public ZCashWalletService {
 public:
  MockZCashWalletService(base::FilePath zcash_data_path,
                         KeyringService& keyring_service,
                         std::unique_ptr<ZCashRpc> zcash_rpc)
      : ZCashWalletService(zcash_data_path,
                           keyring_service,
                           std::move(zcash_rpc)) {}

  MOCK_METHOD1(OnSyncFinished, void(const mojom::AccountIdPtr& account_id));
};

constexpr uint32_t kAccountBirthday = kNu5BlockUpdate + 100u;

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
        base::SequencedTaskRunner::GetCurrentDefault(),
        // base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path.AppendASCII("orchard.db"));

    observer_ = std::make_unique<MockZCashShieldSyncServiceObserver>();
    zcash_wallet_service_ = std::make_unique<MockZCashWalletService>(
        db_path, *keyring_service_,
        std::make_unique<testing::NiceMock<ZCashRpc>>(nullptr, nullptr));

    ResetSyncService();
  }

  void TearDown() override { sync_service_.reset(); }

  void ResetSyncService() {
    zcash_account_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                             mojom::KeyringId::kZCashMainnet,
                                             mojom::AccountKind::kDerived, 0);
    auto account_birthday =
        mojom::ZCashAccountShieldBirthday::New(kAccountBirthday, "hash");
    OrchardFullViewKey fvk;

    sync_service_ = std::make_unique<ZCashShieldSyncService>(
        zcash_wallet_service(),
        ZCashActionContext(zcash_rpc_, OrchardAddrRawPart(), sync_state_,
                           zcash_account_),
        account_birthday, fvk, observer_->GetWeakPtr());

    // Ensure previous OrchardStorage is destroyed on background thread
    task_environment_.RunUntilIdle();
  }

  ZCashShieldSyncService* sync_service() { return sync_service_.get(); }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  MockZCashShieldSyncServiceObserver* observer() { return observer_.get(); }

  mojom::AccountIdPtr account() { return zcash_account_.Clone(); }

  MockZCashWalletService& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  void ApplyScanResults(OrchardBlockScanner::Result&& result) {
    sync_state_.AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account().Clone(), std::move(result));
    task_environment_.RunUntilIdle();
  }

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
          std::vector<OrchardCommitment> commitments;
          std::vector<OrchardNote> notes;
          std::vector<OrchardNoteSpend> spends;

          for (const auto& block : blocks) {
            // 3 notes in the blockchain
            if (block->height == kNu5BlockUpdate + 105) {
              notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 1));
            } else if (block->height == kNu5BlockUpdate + 205) {
              notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 2));
            } else if (block->height == kNu5BlockUpdate + 305) {
              notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 3));
            }

            // First 2 notes are spent
            if (block->height == kNu5BlockUpdate + 255) {
              spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 1)}));
            } else if (block->height == kNu5BlockUpdate + 265) {
              spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 2)}));
            }

            // Last commitment in the block
            commitments.push_back(
                CreateCommitment(CreateMockCommitmentValue(5, block->height),
                                 false, block->height));
          }

          OrchardBlockScanner::Result result = CreateResultForTesting(
              std::move(tree_state), std::move(commitments),
              blocks.back()->height, ToHex(blocks.back()->hash));
          result.discovered_notes = notes;
          result.found_spends = spends;
          std::move(callback).Run(std::move(result));
        }));
  }

 protected:
  mojom::AccountIdPtr zcash_account_;
  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<MockZCashWalletService> zcash_wallet_service_;
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
          [](const std::string& chain_id,
             ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 500u, std::vector<uint8_t>({})));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            // Valid tree state
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabb", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          });

  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault(
          [](const std::string& chain_id, uint32_t from, uint32_t to,
             ZCashRpc::GetCompactBlocksCallback callback) {
            std::vector<zcash::mojom::CompactBlockPtr> blocks;
            for (uint32_t i = from; i <= to; i++) {
              auto chain_metadata = zcash::mojom::ChainMetadata::New();
              chain_metadata->orchard_commitment_tree_size = 0;
              // Create empty block for testing.
              blocks.push_back(zcash::mojom::CompactBlock::New(
                  0u, i, std::vector<uint8_t>({0xbb, 0xaa}),
                  std::vector<uint8_t>(), 0u, std::vector<uint8_t>(),
                  std::vector<zcash::mojom::CompactTxPtr>(),
                  std::move(chain_metadata)));
            }
            std::move(callback).Run(std::move(blocks));
          });

  {
    EXPECT_CALL(zcash_wallet_service(), OnSyncFinished(EqualsMojo(account())));
    sync_service()->StartSyncing(std::nullopt);
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_EQ(sync_status->spendable_balance, 30u);
  }

  testing::Mock::VerifyAndClearExpectations(&zcash_wallet_service());

  // Resume scanning
  ResetSyncService();
  task_environment_.RunUntilIdle();

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [](const std::string& chain_id,
             ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 1000u, std::vector<uint8_t>({})));
          });

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
            std::vector<OrchardCommitment> commitments;
            std::vector<OrchardNote> notes;
            std::vector<OrchardNoteSpend> spends;

            for (const auto& block : blocks) {
              commitments.push_back(
                  CreateCommitment(CreateMockCommitmentValue(1, block->height),
                                   false, block->height));
              // 3 notes in the blockchain
              if (block->height == kNu5BlockUpdate + 605) {
                notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 5));
              } else if (block->height == kNu5BlockUpdate + 705) {
                notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 6));
              } else if (block->height == kNu5BlockUpdate + 905) {
                notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 7));
              }

              // First 2 notes are spent
              if (block->height == kNu5BlockUpdate + 855) {
                spends.push_back(OrchardNoteSpend{
                    block->height, GenerateMockNullifier(account_id, 3)});
              }
            }

            OrchardTreeState orchard_tree_state;
            orchard_tree_state.tree_size = blocks[0]->height - kAccountBirthday;
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(orchard_tree_state), std::move(commitments),
                blocks.back()->height, ToHex(blocks.back()->hash));
            result.discovered_notes = notes;
            result.found_spends = spends;
            std::move(callback).Run(std::move(result));
          })));

  // Continue scanning
  {
    EXPECT_CALL(zcash_wallet_service(), OnSyncFinished(EqualsMojo(account())));
    sync_service()->StartSyncing(std::nullopt);
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_EQ(sync_status->spendable_balance, 180u);
  }

  testing::Mock::VerifyAndClearExpectations(&zcash_wallet_service());

  ResetSyncService();

  // Reorg case, chain tip after the latest scanned block.
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [](const std::string& chain_id,
             ZCashRpc::GetLatestBlockCallback callback) {
            // New chain tip is after the previous one.
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 1100u, std::vector<uint8_t>({})));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            // Hash of the latest scanned block
            // Tree state has been changed
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabbccdd", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          });

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
            std::vector<OrchardCommitment> commitments;
            std::vector<OrchardNote> notes;

            for (const auto& block : blocks) {
              commitments.push_back(
                  CreateCommitment(CreateMockCommitmentValue(1, block->height),
                                   false, block->height));
              if (block->height == kNu5BlockUpdate + 1005u) {
                notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 9));
              }
            }

            OrchardTreeState orchard_tree_state;
            orchard_tree_state.tree_size = blocks[0]->height - kAccountBirthday;
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(orchard_tree_state), std::move(commitments),
                blocks.back()->height, ToHex(blocks.back()->hash));
            result.discovered_notes = notes;
            std::move(callback).Run(std::move(result));
          })));

  {
    EXPECT_CALL(zcash_wallet_service(), OnSyncFinished(EqualsMojo(account())));
    sync_service()->StartSyncing(std::nullopt);
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_EQ(sync_status->spendable_balance, 200u);
  }

  testing::Mock::VerifyAndClearExpectations(&zcash_wallet_service());

  // Reorg case, chain tip before the latest scanned block.
  ResetSyncService();

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [](const std::string& chain_id,
             ZCashRpc::GetLatestBlockCallback callback) {
            // New chain tip below previous one.
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kNu5BlockUpdate + 1030u, std::vector<uint8_t>({})));
          });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            // Hash of the latest scanned block differs
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabbccddee", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          });

  sync_service()->SetOrchardBlockScannerProxyForTesting(
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            std::vector<OrchardCommitment> commitments;
            auto account_id = MakeIndexBasedAccountId(
                mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
                mojom::AccountKind::kDerived, 0);
            std::vector<OrchardNote> discovered_notes;
            std::vector<OrchardNoteSpend> found_spends;
            for (const auto& block : blocks) {
              // Add a new note on height 900
              if (block->height == kNu5BlockUpdate + 1005u) {
                discovered_notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 10));
              }

              // Add a nullifier for previous note
              if (block->height == kNu5BlockUpdate + 1010u) {
                found_spends.push_back(OrchardNoteSpend{
                    block->height, GenerateMockNullifier(account_id, 5)});
              }
              commitments.push_back(
                  CreateCommitment(CreateMockCommitmentValue(2, block->height),
                                   false, block->height));
            }
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::move(commitments),
                blocks.back()->height, ToHex(blocks.back()->hash));
            result.discovered_notes = discovered_notes;
            result.found_spends = found_spends;
            std::move(callback).Run(std::move(result));
          })));

  {
    EXPECT_CALL(zcash_wallet_service(), OnSyncFinished(EqualsMojo(account())));
    sync_service()->StartSyncing(std::nullopt);
    task_environment_.RunUntilIdle();

    auto sync_status = sync_service()->GetSyncStatus();
    EXPECT_TRUE(sync_status);
    EXPECT_EQ(sync_status->spendable_balance, 160u);
  }
  testing::Mock::VerifyAndClearExpectations(&zcash_wallet_service());
}

}  // namespace brave_wallet
