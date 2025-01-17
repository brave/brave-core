/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_verify_chain_state_task.h"

#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
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

constexpr uint32_t kLatestScannedBlock = kNu5BlockUpdate + 10000u;
constexpr char kLatestScannedBlockHash[] = "0x00bbaa";

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
};

}  // namespace

class ZCashVerifyChainStateTaskTest : public testing::Test {
 public:
  ZCashVerifyChainStateTaskTest()
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

    InitSyncState();
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

  base::expected<std::optional<OrchardStorage::AccountMeta>,
                 OrchardStorage::Error>
  GetAccountMeta() {
    std::optional<base::expected<std::optional<OrchardStorage::AccountMeta>,
                                 OrchardStorage::Error>>
        result;
    sync_state_.AsyncCall(&OrchardSyncState::GetAccountMeta)
        .WithArgs(account_id_.Clone())
        .Then(base::BindLambdaForTesting(
            [&](base::expected<std::optional<OrchardStorage::AccountMeta>,
                               OrchardStorage::Error> r) {
              result = std::move(r);
            }));
    task_environment().RunUntilIdle();
    return result.value();
  }

  void InitSyncState() {
    auto lambda = base::BindLambdaForTesting(
        [&](base::expected<OrchardStorage::AccountMeta, OrchardStorage::Error>
                result) { EXPECT_TRUE(result.has_value()); });
    sync_state_.AsyncCall(&OrchardSyncState::RegisterAccount)
        .WithArgs(account_id_.Clone(), kNu5BlockUpdate + 1)
        .Then(std::move(lambda));

    OrchardBlockScanner::Result result = CreateResultForTesting(
        OrchardTreeState(), std::vector<OrchardCommitment>());
    result.discovered_notes.push_back(GenerateMockOrchardNote(
        account_id_, kLatestScannedBlock - kChainReorgBlockDelta - 2, 1));
    result.discovered_notes.push_back(GenerateMockOrchardNote(
        account_id_, kLatestScannedBlock - kChainReorgBlockDelta - 1, 2));
    result.discovered_notes.push_back(GenerateMockOrchardNote(
        account_id_, kLatestScannedBlock - kChainReorgBlockDelta + 1, 3));
    result.discovered_notes.push_back(GenerateMockOrchardNote(
        account_id_, kLatestScannedBlock - kChainReorgBlockDelta + 2, 4));

    sync_state_.AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_.Clone(), std::move(result), kLatestScannedBlock,
                  kLatestScannedBlockHash)
        .Then(base::BindLambdaForTesting(
            [&](base::expected<OrchardStorage::Result, OrchardStorage::Error>
                    r) { EXPECT_TRUE(r.has_value()); }));
    task_environment().RunUntilIdle();
    ASSERT_EQ(GetSpendableNotes().value().size(), 4u);
  }

 private:
  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;
  base::SequenceBound<OrchardSyncState> sync_state_;
  mojom::AccountIdPtr account_id_;
  testing::NiceMock<MockZCashRPC> zcash_rpc_;
};

TEST_F(ZCashVerifyChainStateTaskTest, Reorg_NewChaintip) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kLatestScannedBlock + 1000u, std::vector<uint8_t>({})));
          }));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            if (block->height == kLatestScannedBlock) {
              auto tree_state = zcash::mojom::TreeState::New(
                  chain_id, block->height, "aabb00", 0, "", "");
              std::move(callback).Run(std::move(tree_state));
              return;
            }
            // Valid tree state
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabb", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();

  auto meta_result = GetAccountMeta();
  EXPECT_TRUE(meta_result.has_value());
  EXPECT_EQ(meta_result.value()->latest_scanned_block_id.value(),
            kLatestScannedBlock);
  EXPECT_EQ(meta_result.value()->latest_scanned_block_hash.value(),
            kLatestScannedBlockHash);
  EXPECT_EQ(GetSpendableNotes().value().size(), 4u);
}

TEST_F(ZCashVerifyChainStateTaskTest, NoReorg) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kLatestScannedBlock + 1000u, std::vector<uint8_t>({})));
          }));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            if (block->height == kLatestScannedBlock) {
              auto tree_state = zcash::mojom::TreeState::New(
                  chain_id, block->height, "aabb00", 0, "", "");
              std::move(callback).Run(std::move(tree_state));
              return;
            }
            // Valid tree state
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabb", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();

  auto meta_result = GetAccountMeta();
  EXPECT_TRUE(meta_result.has_value());
  EXPECT_EQ(meta_result.value()->latest_scanned_block_id.value(),
            kLatestScannedBlock);
  EXPECT_EQ(meta_result.value()->latest_scanned_block_hash.value(),
            kLatestScannedBlockHash);
  EXPECT_EQ(GetSpendableNotes().value().size(), 4u);
}

TEST_F(ZCashVerifyChainStateTaskTest, Reorg_ChainTipBeforeLatestScannedBlock) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kLatestScannedBlock - 1u, std::vector<uint8_t>({})));
          }));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            if (block->height == kLatestScannedBlock) {
              auto tree_state = zcash::mojom::TreeState::New(
                  chain_id, block->height, "aabb00", 0, "", "");
              std::move(callback).Run(std::move(tree_state));
              return;
            }
            if (block->height ==
                kLatestScannedBlock - 1 - kChainReorgBlockDelta) {
              auto tree_state = zcash::mojom::TreeState::New(
                  chain_id, block->height, "aabb0022", 0, "", "");
              std::move(callback).Run(std::move(tree_state));
              return;
            }
            // Valid tree state
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabb", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();

  auto meta_result = GetAccountMeta();
  EXPECT_TRUE(meta_result.has_value());
  EXPECT_EQ(meta_result.value()->latest_scanned_block_id.value(),
            kLatestScannedBlock - 1 - kChainReorgBlockDelta);
  EXPECT_EQ(meta_result.value()->latest_scanned_block_hash.value(),
            "0x2200bbaa");
  EXPECT_EQ(GetSpendableNotes().value().size(), 2u);
}

TEST_F(ZCashVerifyChainStateTaskTest, Reorg_ChainTipAfterLatestScannedBlock) {
  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kLatestScannedBlock + 1000u, std::vector<uint8_t>({})));
          }));

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault(::testing::Invoke(
          [](const std::string& chain_id, zcash::mojom::BlockIDPtr block,
             ZCashRpc::GetTreeStateCallback callback) {
            if (block->height == kLatestScannedBlock) {
              // Hash differs from the latest scanned block hash
              auto tree_state = zcash::mojom::TreeState::New(
                  chain_id, block->height, "aabb0011", 0, "", "");
              std::move(callback).Run(std::move(tree_state));
              return;
            }
            // New tree state for the new latest scanned block
            if (block->height == kLatestScannedBlock - kChainReorgBlockDelta) {
              // Hash differs from the latest scanned block hash
              auto tree_state = zcash::mojom::TreeState::New(
                  chain_id, block->height, "aabb0022", 0, "", "");
              std::move(callback).Run(std::move(tree_state));
              return;
            }
            // Valid tree state
            auto tree_state = zcash::mojom::TreeState::New(
                chain_id, block->height, "aabb", 0, "", "");
            std::move(callback).Run(std::move(tree_state));
          }));

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_TRUE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();

  auto meta_result = GetAccountMeta();
  EXPECT_TRUE(meta_result.has_value());
  EXPECT_EQ(meta_result.value()->latest_scanned_block_id.value(),
            kLatestScannedBlock - kChainReorgBlockDelta);
  EXPECT_EQ(meta_result.value()->latest_scanned_block_hash.value(),
            "0x2200bbaa");
  EXPECT_EQ(GetSpendableNotes().value().size(), 2u);
}

}  // namespace brave_wallet
