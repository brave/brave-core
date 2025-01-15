/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_blocks_batch_scan_task.h"

#include <memory>
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

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

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

class ZCashBlocksBatchScanTest : public testing::Test {
 public:
  ZCashBlocksBatchScanTest()
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
              // Only 600 blocks available
              if (to > kNu5BlockUpdate + 600u) {
                std::move(callback).Run(base::unexpected("error"));
                return;
              }
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

            if (block->height == kNu5BlockUpdate + 255) {
              result.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 1)}));
            } else if (block->height == kNu5BlockUpdate + 265) {
              result.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 2)}));
            }

            if (block->height == kNu5BlockUpdate + 405) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 4));
            } else if (block->height == kNu5BlockUpdate + 505) {
              result.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 5));
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

TEST_F(ZCashBlocksBatchScanTest, SingleBlockDecoded) {
  ZCashActionContext context = CreateContext();

  std::vector<uint32_t> decoded_blocks;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::vector<uint32_t>* decoded_blocks, OrchardTreeState tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>());
            for (const auto& block : blocks) {
              decoded_blocks->push_back(block->height);
            }
            std::move(callback).Run(std::move(result));
          },
          &decoded_blocks));

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(decoded_blocks.size(), 1u);
            EXPECT_EQ(decoded_blocks[0], kNu5BlockUpdate + 1);
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 1, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashBlocksBatchScanTest, AllBlocksDecoded) {
  ZCashActionContext context = CreateContext();

  std::vector<uint32_t> decoded_blocks;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::vector<uint32_t>* decoded_blocks, OrchardTreeState tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>());
            for (const auto& block : blocks) {
              decoded_blocks->push_back(block->height);
            }
            std::move(callback).Run(std::move(result));
          },
          &decoded_blocks));

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
            // We shouldn't have any notes added since one block is corrupted
            EXPECT_EQ(decoded_blocks.size(), 400u);
            for (int i = 0; i < 400; i++) {
              EXPECT_EQ(decoded_blocks[i], i + 1 + kNu5BlockUpdate);
            }
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 400, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashBlocksBatchScanTest, Scan) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
            EXPECT_EQ(GetSpendableNotes().value().size(), 2u);
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 500, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashBlocksBatchScanTest, Error_PartialScan) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
            // Since we had only 600 available blocks and requiested to scan 700
            // We shouldn't have any notes added to the database.
            EXPECT_EQ(GetSpendableNotes().value().size(), 0u);
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 700, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashBlocksBatchScanTest, Error_PartialDecoding) {
  ZCashActionContext context = CreateContext();

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
            for (const auto& block : blocks) {
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

              if (block->height == kNu5BlockUpdate + 355) {
                std::move(callback).Run(base::unexpected(
                    OrchardBlockScanner::ErrorCode::kDecoderError));
                return;
              }
            }
            std::move(callback).Run(std::move(result));
          }));

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
            // We shouldn't have any notes added since one block is corrupted
            EXPECT_EQ(GetSpendableNotes().value().size(), 0u);
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 400, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashBlocksBatchScanTest, NetworkError_Blocks) {
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, uint32_t from, uint32_t to,
             ZCashRpc::GetCompactBlocksCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 200, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashBlocksBatchScanTest, NetworkError_TreeState) {
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 200, callback.Get());
  task.Start();
  task_environment().RunUntilIdle();
}

TEST_F(ZCashBlocksBatchScanTest, DecodingError) {
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

  base::MockCallback<ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillOnce(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
          });

  auto task =
      ZCashBlocksBatchScanTask(context, *block_scanner, kNu5BlockUpdate + 1,
                               kNu5BlockUpdate + 200, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

}  // namespace brave_wallet
