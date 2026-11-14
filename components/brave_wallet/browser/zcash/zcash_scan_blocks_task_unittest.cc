/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_scan_blocks_task.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/notreached.h"
#include "base/run_loop.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {

constexpr uint32_t kExpectedBatchSize = 1024;
constexpr uint32_t kChainTipHeight = kNu5BlockUpdate + 10000;

using ScanResult = base::expected<ZCashShieldSyncService::ScanRangeResult,
                                  ZCashShieldSyncService::Error>;

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

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
};

}  // namespace

class ZCashScanBlocksTaskTest : public testing::Test {
 public:
  ZCashScanBlocksTaskTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    sync_state_.emplace(
        base::SequencedTaskRunner::GetCurrentDefault(),
        std::make_unique<OrchardSyncState>(temp_dir_.GetPath()));

    account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                          mojom::KeyringId::kZCashMainnet,
                                          mojom::AccountKind::kDerived, 0);
    base::test::TestFuture<
        base::expected<OrchardStorage::Result, OrchardStorage::Error>>
        register_future;
    sync_state_.AsyncCall(&OrchardSyncState::RegisterAccount)
        .WithArgs(account_id_.Clone(), kNu5BlockUpdate + 1)
        .Then(register_future.GetCallback());
    auto register_result = register_future.Take();
    ASSERT_TRUE(register_result.has_value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess, register_result.value());

    InitZCashRpc();
  }

  void TearDown() override { sync_state_.SynchronouslyResetForTest(); }

  void InitZCashRpc() {
    ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
        .WillByDefault([](const std::string& chain_id,
                          ZCashRpc::GetLatestBlockCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          std::move(callback).Run(zcash::mojom::BlockID::New(
              kChainTipHeight, std::vector<uint8_t>({})));
        });

    ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
        .WillByDefault([](const std::string& chain_id,
                          zcash::mojom::BlockIDPtr block,
                          ZCashRpc::GetTreeStateCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          // Valid tree state
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb", 0, "", "", "");
          std::move(callback).Run(std::move(tree_state));
        });

    ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
        .WillByDefault([](const std::string& chain_id, uint32_t from,
                          uint32_t to,
                          ZCashRpc::GetCompactBlocksCallback callback) {
          EXPECT_EQ(chain_id, mojom::kZCashMainnet);
          std::vector<zcash::mojom::CompactBlockPtr> blocks;
          for (uint32_t i = from; i <= to; i++) {
            auto chain_metadata = zcash::mojom::ChainMetadata::New();
            chain_metadata->orchard_commitment_tree_size = 0;
            // Create empty block for testing
            blocks.push_back(zcash::mojom::CompactBlock::New(
                0u, i, std::vector<uint8_t>({0xbb, 0xaa}),
                std::vector<uint8_t>(), 0u, std::vector<uint8_t>(),
                std::vector<zcash::mojom::CompactTxPtr>(),
                std::move(chain_metadata)));
          }
          std::move(callback).Run(std::move(blocks));
        });
  }

  ZCashActionContext CreateContext() {
    return ZCashActionContext(zcash_rpc_, {}, sync_state_, account_id_);
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  // Returns std::nullopt (after recording a failure) if the storage returned an
  // error or has no notes bundle. May be called from inside an observer
  // callback, hence the nestable wait.
  std::optional<OrchardSyncState::SpendableNotesBundle> GetSpendableNotes() {
    base::test::TestFuture<
        base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                       OrchardStorage::Error>>
        future;
    sync_state_.AsyncCall(&OrchardSyncState::GetSpendableNotes)
        .WithArgs(OrchardPool::kOrchard, account_id_.Clone(),
                  OrchardAddrRawPart({}))
        .Then(future.GetCallback());
    if (!future.Wait(base::RunLoop::Type::kNestableTasksAllowed)) {
      ADD_FAILURE() << "Timed out waiting for spendable notes";
      return std::nullopt;
    }
    auto result = future.Take();
    if (!result.has_value() || !result.value().has_value()) {
      ADD_FAILURE() << "Failed to get spendable notes";
      return std::nullopt;
    }
    return std::move(result.value());
  }

  std::unique_ptr<MockOrchardBlockScannerProxy>
  CreateMockOrchardBlockScannerProxy() {
    return std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
        [](OrchardTreeState tree_state,
           std::optional<OrchardTreeState> ironwood_tree_state,
           std::vector<zcash::mojom::CompactBlockPtr> blocks,
           base::OnceCallback<void(
               base::expected<OrchardBlockScanner::Result,
                              OrchardBlockScanner::ErrorCode>)> callback) {
          auto account_id = MakeIndexBasedAccountId(
              mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
              mojom::AccountKind::kDerived, 0);
          OrchardBlockScanner::Result result = CreateResultForTesting(
              std::move(tree_state), std::vector<OrchardCommitment>(),
              blocks[blocks.size() - 1]->height,
              ToHex(blocks[blocks.size() - 1]->hash));
          for (const auto& block : blocks) {
            // 3 notes in the blockchain found in the first batch
            if (block->height == kNu5BlockUpdate + 105) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 1));
            } else if (block->height == kNu5BlockUpdate + 205) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 2));
            } else if (block->height == kNu5BlockUpdate + 305) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 3));
            }

            // First 2 notes are spent in the second batch
            if (block->height == kNu5BlockUpdate + kExpectedBatchSize + 255) {
              result.orchard.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 1)}));
            } else if (block->height ==
                       kNu5BlockUpdate + kExpectedBatchSize + 265) {
              result.orchard.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 2)}));
            }

            // Another 2 additional notes found in the third batch
            if (block->height ==
                kNu5BlockUpdate + kExpectedBatchSize * 2 + 105) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 4));
            } else if (block->height ==
                       kNu5BlockUpdate + kExpectedBatchSize * 2 + 205) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 5));
            }

            // Another 2 additional notes found far away from 3 previous batches
            if (block->height ==
                kNu5BlockUpdate + kExpectedBatchSize * 7 + 105) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 6));
            } else if (block->height ==
                       kNu5BlockUpdate + kExpectedBatchSize * 7 + 205) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 7));
            }
          }
          std::move(callback).Run(std::move(result));
        }));
  }

 private:
  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;
  OrchardSyncState::SequenceBound sync_state_;
  mojom::AccountIdPtr account_id_;
  testing::NiceMock<MockZCashRPC> zcash_rpc_;
};

TEST_F(ZCashScanBlocksTaskTest, ScanRanges) {
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            if (blocks[0]->height == kNu5BlockUpdate + 1) {
              EXPECT_EQ(blocks.size(), kExpectedBatchSize);
              EXPECT_EQ(blocks.back()->height,
                        kNu5BlockUpdate + kExpectedBatchSize);
            } else if (blocks[0]->height ==
                       kNu5BlockUpdate + kExpectedBatchSize + 1) {
              EXPECT_EQ(blocks.size(), kExpectedBatchSize);
              EXPECT_EQ(blocks.back()->height,
                        kNu5BlockUpdate + kExpectedBatchSize * 2);
            } else if (blocks[0]->height ==
                       kNu5BlockUpdate + kExpectedBatchSize * 2 + 1) {
              EXPECT_EQ(blocks.size(), kExpectedBatchSize);
              EXPECT_EQ(blocks.back()->height,
                        kNu5BlockUpdate + kExpectedBatchSize * 3);
            } else if (blocks[0]->height ==
                       kNu5BlockUpdate + kExpectedBatchSize * 3 + 1) {
              EXPECT_EQ(blocks.size(), 15u);
              EXPECT_EQ(blocks.back()->height,
                        kNu5BlockUpdate + kExpectedBatchSize * 3 + 15);
            } else {
              NOTREACHED();
            }
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks[blocks.size() - 1]->height,
                ToHex(blocks[blocks.size() - 1]->hash));
            std::move(callback).Run(std::move(result));
          }));

  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  testing::Sequence seq;
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 0u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 1u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 2u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 3u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        base::ScopedClosureRunner quit(run_loop.QuitClosure());
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 4u);
      });

  auto task =
      ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                          kNu5BlockUpdate + kExpectedBatchSize * 3 + 15);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, ScanSingle) {
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            auto account_id = MakeIndexBasedAccountId(
                mojom::CoinType::ZEC, mojom::KeyringId::kZCashMainnet,
                mojom::AccountKind::kDerived, 0);
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks[blocks.size() - 1]->height,
                ToHex(blocks[blocks.size() - 1]->hash));
            EXPECT_EQ(blocks.size(), 1u);
            EXPECT_EQ(blocks[0]->height, kNu5BlockUpdate + 1u);
            result.orchard.discovered_notes.push_back(
                GenerateMockOrchardNote(account_id, blocks[0]->height, 1));
            std::move(callback).Run(std::move(result));
          }));

  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  testing::Sequence seq;
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 0u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        base::ScopedClosureRunner quit(run_loop.QuitClosure());
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 1u);
        auto notes1 = GetSpendableNotes();
        ASSERT_TRUE(notes1.has_value());
        EXPECT_EQ(1u, notes1->all_notes.size());
      });

  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kNu5BlockUpdate + 1);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, ScanLimited) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  testing::Sequence seq;
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 0u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 1u);
        auto notes1 = GetSpendableNotes();
        ASSERT_TRUE(notes1.has_value());
        EXPECT_EQ(3u, notes1->all_notes.size());
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 2u);
        auto notes2 = GetSpendableNotes();
        ASSERT_TRUE(notes2.has_value());
        EXPECT_EQ(1u, notes2->all_notes.size());
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        base::ScopedClosureRunner quit(run_loop.QuitClosure());
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 3u);
        auto notes3 = GetSpendableNotes();
        ASSERT_TRUE(notes3.has_value());
        EXPECT_EQ(3u, notes3->all_notes.size());
      });

  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kNu5BlockUpdate + kExpectedBatchSize * 3);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, ScanUnlimited) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  testing::Sequence seq;
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 0u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 1u);
        auto notes1 = GetSpendableNotes();
        ASSERT_TRUE(notes1.has_value());
        EXPECT_EQ(3u, notes1->all_notes.size());
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 2u);
        auto notes2 = GetSpendableNotes();
        ASSERT_TRUE(notes2.has_value());
        EXPECT_EQ(1u, notes2->all_notes.size());
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 3u);
        auto notes3 = GetSpendableNotes();
        ASSERT_TRUE(notes3.has_value());
        EXPECT_EQ(3u, notes3->all_notes.size());
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 4u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 5u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 6u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 7u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 8u);
        auto notes8 = GetSpendableNotes();
        ASSERT_TRUE(notes8.has_value());
        EXPECT_EQ(5u, notes8->all_notes.size());
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 9u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        base::ScopedClosureRunner quit(run_loop.QuitClosure());
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 10u);
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, PartialScanningDueError) {
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::vector<zcash::mojom::CompactBlockPtr> blocks;
        // Blocks after the second batch are failing
        if (from > kNu5BlockUpdate + kExpectedBatchSize + 10) {
          std::move(callback).Run(base::unexpected("error"));
          return;
        }
        for (uint32_t i = from; i <= to; i++) {
          auto chain_metadata = zcash::mojom::ChainMetadata::New();
          chain_metadata->orchard_commitment_tree_size = 0;
          // Create empty block for testing
          blocks.push_back(zcash::mojom::CompactBlock::New(
              0u, i, std::vector<uint8_t>({0xbb, 0xaa}), std::vector<uint8_t>(),
              0u, std::vector<uint8_t>(),
              std::vector<zcash::mojom::CompactTxPtr>(),
              std::move(chain_metadata)));
        }
        std::move(callback).Run(std::move(blocks));
      });

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  testing::Sequence seq;
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 0u);
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->ready_ranges, 1u);
        auto notes1 = GetSpendableNotes();
        ASSERT_TRUE(notes1.has_value());
        EXPECT_EQ(3u, notes1->all_notes.size());
      });
  EXPECT_CALL(callback, Run(testing::_))
      .InSequence(seq)
      .WillOnce([&](ScanResult result) {
        base::ScopedClosureRunner quit(run_loop.QuitClosure());
        EXPECT_FALSE(result.has_value());
      });

  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kNu5BlockUpdate + kExpectedBatchSize * 3);
  // Sequental scanning
  task.set_max_tasks_in_progress(1);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, ChainTipMismatch) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kChainTipHeight - 200, std::vector<uint8_t>({})));
      });

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](ScanResult result) {
        base::ScopedClosureRunner quit(run_loop.QuitClosure());
        EXPECT_TRUE(!result.has_value());
      });

  // Scan with right border less than actual chain tip
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kChainTipHeight);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, NetworkError_LatestBlock) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](ScanResult result) {
        base::ScopedClosureRunner quit(run_loop.QuitClosure());
        EXPECT_TRUE(!result.has_value());
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, NetworkError_CompactBlocks) {
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  int calls = 0;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(2)
      .WillRepeatedly([&](ScanResult result) {
        EXPECT_TRUE(!result.has_value() || result.value().ready_ranges == 0u);
        if (++calls == 2) {
          run_loop.Quit();
        }
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, NetworkError_TreeState) {
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  int calls = 0;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(2)
      .WillRepeatedly([&](ScanResult result) {
        EXPECT_TRUE(!result.has_value() || result.value().ready_ranges == 0u);
        if (++calls == 2) {
          run_loop.Quit();
        }
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  run_loop.Run();
}

TEST_F(ZCashScanBlocksTaskTest, DecodingError) {
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            std::move(callback).Run(base::unexpected(
                OrchardBlockScanner::ErrorCode::kDecoderError));
          }));
  ZCashActionContext context = CreateContext();

  base::RunLoop run_loop;
  int calls = 0;
  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(2)
      .WillRepeatedly([&](ScanResult result) {
        EXPECT_TRUE(!result.has_value() || result.value().ready_ranges == 0u);
        if (++calls == 2) {
          run_loop.Quit();
        }
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  run_loop.Run();
}

}  // namespace brave_wallet
