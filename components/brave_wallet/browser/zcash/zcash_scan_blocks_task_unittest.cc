/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_scan_blocks_task.h"

#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "content/public/test/browser_task_environment.h"
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
    base::FilePath db_path(
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
    sync_state_.emplace(base::SequencedTaskRunner::GetCurrentDefault(),
                        db_path.AppendASCII("orchard.db"));
    account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                          mojom::KeyringId::kZCashMainnet,
                                          mojom::AccountKind::kDerived, 0);
    auto lambda = base::BindLambdaForTesting(
        [&](base::expected<OrchardStorage::AccountMeta, OrchardStorage::Error>
                result) { EXPECT_TRUE(result.has_value()); });
    sync_state_.AsyncCall(&OrchardSyncState::RegisterAccount)
        .WithArgs(account_id_.Clone(), kNu5BlockUpdate + 1)
        .Then(std::move(lambda));

    InitZCashRpc();
  }

  void InitZCashRpc() {
    ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
        .WillByDefault(
            ::testing::Invoke([](const std::string& chain_id,
                                 ZCashRpc::GetLatestBlockCallback callback) {
              std::move(callback).Run(zcash::mojom::BlockID::New(
                  kChainTipHeight, std::vector<uint8_t>({})));
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
            }));
  }

  ZCashActionContext CreateContext() {
    return ZCashActionContext(zcash_rpc_, sync_state_, account_id_,
                              mojom::kZCashMainnet);
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  base::expected<std::vector<OrchardNote>, OrchardStorage::Error>
  GetSpendableNotes() {
    std::optional<
        base::expected<std::vector<OrchardNote>, OrchardStorage::Error>>
        result;
    sync_state_.AsyncCall(&OrchardSyncState::GetSpendableNotes)
        .WithArgs(account_id_.Clone())
        .Then(base::BindLambdaForTesting(
            [&](base::expected<std::vector<OrchardNote>, OrchardStorage::Error>
                    r) { result = std::move(r); }));
    task_environment().RunUntilIdle();
    return result.value();
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
          OrchardBlockScanner::Result result = CreateResultForTesting(
              std::move(tree_state), std::vector<OrchardCommitment>());
          for (const auto& block : blocks) {
            // 3 notes in the blockchain found in the first batch
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

            // First 2 notes are spent in the second batch
            if (block->height == kNu5BlockUpdate + kExpectedBatchSize + 255) {
              result.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 1)}));
            } else if (block->height ==
                       kNu5BlockUpdate + kExpectedBatchSize + 265) {
              result.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 2)}));
            }

            // Another 2 additional notes found in the third batch
            if (block->height ==
                kNu5BlockUpdate + kExpectedBatchSize * 2 + 105) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 4));
            } else if (block->height ==
                       kNu5BlockUpdate + kExpectedBatchSize * 2 + 205) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 5));
            }

            // Another 2 additional notes found far away from 3 previous batches
            if (block->height ==
                kNu5BlockUpdate + kExpectedBatchSize * 7 + 105) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 6));
            } else if (block->height ==
                       kNu5BlockUpdate + kExpectedBatchSize * 7 + 205) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 7));
            }
          }
          std::move(callback).Run(std::move(result));
        }));
  }

 private:
  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;
  base::SequenceBound<OrchardSyncState> sync_state_;
  mojom::AccountIdPtr account_id_;
  testing::NiceMock<MockZCashRPC> zcash_rpc_;
};

TEST_F(ZCashScanBlocksTaskTest, ScanRanges) {
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
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
                std::move(tree_state), std::vector<OrchardCommitment>());
            std::move(callback).Run(std::move(result));
          }));

  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(4)
      .WillRepeatedly(
          [&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                             ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
          });

  auto task =
      ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                          kNu5BlockUpdate + kExpectedBatchSize * 3 + 15);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, ScanSingle) {
  auto block_scanner =
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
            EXPECT_EQ(blocks.size(), 1u);
            EXPECT_EQ(blocks[0]->height, kNu5BlockUpdate + 1u);
            result.discovered_notes.push_back(
                GenerateMockOrchardNote(account_id, blocks[0]->height, 1));
            std::move(callback).Run(std::move(result));
          }));

  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                             ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
            if (result->ready_ranges == 1u) {
              auto notes1 = GetSpendableNotes();
              EXPECT_EQ(1u, notes1.value().size());
            }
          });

  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kNu5BlockUpdate + 1);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, ScanLimited) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(3)
      .WillRepeatedly(
          [&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                             ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
            if (result->ready_ranges == 1u) {
              auto notes1 = GetSpendableNotes();
              EXPECT_EQ(3u, notes1.value().size());
            } else if (result->ready_ranges == 2u) {
              auto notes2 = GetSpendableNotes();
              EXPECT_EQ(1u, notes2.value().size());
            } else if (result->ready_ranges == 3u) {
              auto notes3 = GetSpendableNotes();
              EXPECT_EQ(3u, notes3.value().size());
            }
          });

  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kNu5BlockUpdate + kExpectedBatchSize * 3);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, ScanUnlimited) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(10)
      .WillRepeatedly(
          [&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                             ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
            if (result->ready_ranges == 1u) {
              auto notes1 = GetSpendableNotes();
              EXPECT_EQ(3u, notes1.value().size());
            } else if (result->ready_ranges == 2u) {
              auto notes2 = GetSpendableNotes();
              EXPECT_EQ(1u, notes2.value().size());
            } else if (result->ready_ranges == 3u) {
              auto notes3 = GetSpendableNotes();
              EXPECT_EQ(3u, notes3.value().size());
            } else if (result->ready_ranges == 8u) {
              auto notes8 = GetSpendableNotes();
              EXPECT_EQ(5u, notes8.value().size());
            }
          });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, PartialScanningDueError) {
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, uint32_t from, uint32_t to,
             ZCashRpc::GetCompactBlocksCallback callback) {
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
                  0u, i, std::vector<uint8_t>({0xbb, 0xaa}),
                  std::vector<uint8_t>(), 0u, std::vector<uint8_t>(),
                  std::vector<zcash::mojom::CompactTxPtr>(),
                  std::move(chain_metadata)));
            }
            std::move(callback).Run(std::move(blocks));
          }));

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(2)
      .WillRepeatedly(
          [&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                             ZCashShieldSyncService::Error> result) {
            // First batch completes, second batch fails
            EXPECT_TRUE(!result.has_value() || result->ready_ranges == 1u);
            if (result.has_value()) {
              auto notes1 = GetSpendableNotes();
              EXPECT_EQ(3u, notes1.value().size());
            }
          });

  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kNu5BlockUpdate + kExpectedBatchSize * 3);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, ChainTipMismatch) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kChainTipHeight - 200, std::vector<uint8_t>({})));
          }));

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                                   ZCashShieldSyncService::Error> result) {
        EXPECT_FALSE(result.has_value());
      });

  // Scan with right border less than actual chain tip
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  kChainTipHeight);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, NetworkError_LatestBlock) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                                   ZCashShieldSyncService::Error> result) {
        EXPECT_FALSE(result.has_value());
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, NetworkError_CompactBlocks) {
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, uint32_t from, uint32_t to,
             ZCashRpc::GetCompactBlocksCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                                   ZCashShieldSyncService::Error> result) {
        EXPECT_FALSE(result.has_value());
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, NetworkError_TreeState) {
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                                   ZCashShieldSyncService::Error> result) {
        EXPECT_FALSE(result.has_value());
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashScanBlocksTaskTest, DecodingError) {
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            std::move(callback).Run(base::unexpected(
                OrchardBlockScanner::ErrorCode::kDecoderError));
          }));
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashScanBlocksTask::ZCashScanBlocksTaskObserver> callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce([&](base::expected<ZCashShieldSyncService::ScanRangeResult,
                                   ZCashShieldSyncService::Error> result) {
        EXPECT_FALSE(result.has_value());
      });

  // Scan without right border
  auto task = ZCashScanBlocksTask(context, *block_scanner, callback.Get(),
                                  std::nullopt);
  task.Start();

  task_environment().RunUntilIdle();
}

}  // namespace brave_wallet
