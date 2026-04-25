/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_verify_chain_state_task.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
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

class MockOrchardSyncState : public OrchardSyncState {
 public:
  using OrchardSyncState::OrchardSyncState;
  ~MockOrchardSyncState() override {}

  MOCK_METHOD1(GetMinCheckpointId,
               base::expected<std::optional<uint32_t>, OrchardStorage::Error>(
                   const mojom::AccountIdPtr& account_id));

  MOCK_METHOD3(Rewind,
               base::expected<OrchardStorage::Result, OrchardStorage::Error>(
                   const mojom::AccountIdPtr& account_id,
                   uint32_t rewind_block_height,
                   const std::string& rewind_block_hash));
};

}  // namespace

class ZCashVerifyChainStateTaskTest : public testing::Test {
 public:
  ZCashVerifyChainStateTaskTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                          mojom::KeyringId::kZCashMainnet,
                                          mojom::AccountKind::kDerived, 0);

    auto mocked_sync_state =
        std::make_unique<MockOrchardSyncState>(temp_dir_.GetPath());
    mocked_sync_state_ptr_ = mocked_sync_state.get();
    sync_state_ = OrchardSyncState::SequenceBound(
        OrchardSyncState::CreateSyncStateSequence(),
        std::move(mocked_sync_state));
    InitSyncState();
  }

  void InitSyncState() {
    auto lambda = base::BindLambdaForTesting(
        [&](base::expected<OrchardStorage::Result, OrchardStorage::Error>
                result) {
          EXPECT_EQ(OrchardStorage::Result::kSuccess, result.value());
        });
    sync_state_.AsyncCall(&OrchardSyncState::RegisterAccount)
        .WithArgs(account_id_.Clone(), kNu5BlockUpdate + 1)
        .Then(std::move(lambda));

    OrchardBlockScanner::Result result = CreateResultForTesting(
        OrchardTreeState(), std::vector<OrchardCommitment>(),
        kLatestScannedBlock, kLatestScannedBlockHash);
    sync_state_.AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(account_id_.Clone(), std::move(result))
        .Then(base::BindLambdaForTesting(
            [&](base::expected<OrchardStorage::Result, OrchardStorage::Error>
                    r) { EXPECT_TRUE(r.has_value()); }));
    task_environment().RunUntilIdle();
  }

  ZCashActionContext CreateContext() {
    return ZCashActionContext(zcash_rpc_, {}, sync_state_, account_id_);
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  MockOrchardSyncState& mocked_sync_state() { return *mocked_sync_state_ptr_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

 private:
  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;
  mojom::AccountIdPtr account_id_;
  testing::NiceMock<MockZCashRPC> zcash_rpc_;
  OrchardSyncState::SequenceBound sync_state_;
  raw_ptr<MockOrchardSyncState> mocked_sync_state_ptr_ = nullptr;
};

TEST_F(ZCashVerifyChainStateTaskTest, NoReorg) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::ok(std::optional<uint32_t>(kLatestScannedBlock - 100u));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kLatestScannedBlock + 1000u, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block->height == kLatestScannedBlock) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb00", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        // Valid tree state
        auto tree_state = zcash::mojom::TreeState::New(chain_id, block->height,
                                                       "aabb", 0, "", "");
        std::move(callback).Run(std::move(tree_state));
      });

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

  EXPECT_CALL(mocked_sync_state(), Rewind(_, _, _)).Times(0);

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  task_environment().RunUntilIdle();
}

TEST_F(ZCashVerifyChainStateTaskTest, Reorg_ChainTipBeforeLatestScannedBlock) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::ok(std::optional<uint32_t>(kLatestScannedBlock - 100u));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kLatestScannedBlock - 1u, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block->height == kLatestScannedBlock - 100) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb0022", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        if (block->height == kLatestScannedBlock) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb00", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }

        // Valid tree state
        auto tree_state = zcash::mojom::TreeState::New(chain_id, block->height,
                                                       "aabb", 0, "", "");
        std::move(callback).Run(std::move(tree_state));
      });

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

  EXPECT_CALL(mocked_sync_state(),
              Rewind(EqualsMojo(account_id()), Eq(kLatestScannedBlock - 100u),
                     Eq("0x2200bbaa")))
      .Times(1);

  task_environment().RunUntilIdle();
}

TEST_F(ZCashVerifyChainStateTaskTest, Reorg_ChainTipAfterLatestScannedBlock) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::ok(std::optional<uint32_t>(kLatestScannedBlock - 100u));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kLatestScannedBlock + 1000u, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block->height == kLatestScannedBlock) {
          // Hash differs from the latest scanned block hash
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb0011", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        // New tree state for the new latest scanned block
        if (block->height == kLatestScannedBlock - 100) {
          // Hash differs from the latest scanned block hash
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb0022", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        // Valid tree state
        auto tree_state = zcash::mojom::TreeState::New(chain_id, block->height,
                                                       "aabb", 0, "", "");
        std::move(callback).Run(std::move(tree_state));
      });

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

  EXPECT_CALL(mocked_sync_state(),
              Rewind(EqualsMojo(account_id()), Eq(kLatestScannedBlock - 100u),
                     Eq("0x2200bbaa")))
      .Times(1);

  task_environment().RunUntilIdle();
}

TEST_F(ZCashVerifyChainStateTaskTest, Reorg_LatestBlockHashChanged) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::ok(std::optional<uint32_t>(kLatestScannedBlock - 100u));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kLatestScannedBlock, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block->height == kLatestScannedBlock - 100) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "1122", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        if (block->height == kLatestScannedBlock) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb33", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        // Valid tree state
        auto tree_state = zcash::mojom::TreeState::New(chain_id, block->height,
                                                       "aabb", 0, "", "");
        std::move(callback).Run(std::move(tree_state));
      });

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

  EXPECT_CALL(mocked_sync_state(),
              Rewind(EqualsMojo(account_id()), Eq(kLatestScannedBlock - 100u),
                     Eq("0x2211")))
      .Times(1);

  task_environment().RunUntilIdle();
}

TEST_F(ZCashVerifyChainStateTaskTest, Error_CheckpointIdFailed) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::unexpected(OrchardStorage::Error());
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kLatestScannedBlock, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block->height == kLatestScannedBlock) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb33", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        // Valid tree state
        auto tree_state = zcash::mojom::TreeState::New(chain_id, block->height,
                                                       "aabb", 0, "", "");
        std::move(callback).Run(std::move(tree_state));
      });

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  EXPECT_CALL(mocked_sync_state(), Rewind(_, _, _)).Times(0);

  task_environment().RunUntilIdle();
}

TEST_F(ZCashVerifyChainStateTaskTest, Error_NoCheckpointId) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::ok(std::nullopt);
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kLatestScannedBlock, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block->height == kLatestScannedBlock) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb33", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        // Valid tree state
        auto tree_state = zcash::mojom::TreeState::New(chain_id, block->height,
                                                       "aabb", 0, "", "");
        std::move(callback).Run(std::move(tree_state));
      });

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  EXPECT_CALL(mocked_sync_state(), Rewind(_, _, _)).Times(0);

  task_environment().RunUntilIdle();
}

TEST_F(ZCashVerifyChainStateTaskTest, Error_LatestBlockFailed) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::ok(std::optional<uint32_t>(kLatestScannedBlock - 100u));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        if (block->height == kLatestScannedBlock - 100) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "1122", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        if (block->height == kLatestScannedBlock) {
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb00", 0, "", "");
          std::move(callback).Run(std::move(tree_state));
          return;
        }
        // Valid tree state
        auto tree_state = zcash::mojom::TreeState::New(chain_id, block->height,
                                                       "aabb", 0, "", "");
        std::move(callback).Run(std::move(tree_state));
      });

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  EXPECT_CALL(mocked_sync_state(), Rewind(_, _, _)).Times(0);

  task_environment().RunUntilIdle();
}

TEST_F(ZCashVerifyChainStateTaskTest, Error_TreeStateFailed) {
  ON_CALL(mocked_sync_state(), GetMinCheckpointId(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        return base::ok(std::optional<uint32_t>(kLatestScannedBlock - 100u));
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(zcash::mojom::BlockID::New(
            kLatestScannedBlock, std::vector<uint8_t>({})));
      });

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  ZCashActionContext context = CreateContext();

  base::MockCallback<
      ZCashVerifyChainStateTask::ZCashVerifyChainStateTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<bool, ZCashShieldSyncService::Error> result) {
            EXPECT_FALSE(result.has_value());
          });

  auto task = ZCashVerifyChainStateTask(context, callback.Get());
  task.Start();

  EXPECT_CALL(mocked_sync_state(), Rewind(_, _, _)).Times(0);

  task_environment().RunUntilIdle();
}

}  // namespace brave_wallet
