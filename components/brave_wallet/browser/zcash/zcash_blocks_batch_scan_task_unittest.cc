/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_blocks_batch_scan_task.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
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
    sync_state_.emplace(OrchardSyncState::CreateSyncStateSequence(),
                        OrchardSyncState::CreateSyncState(temp_dir_.GetPath()));

    account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                          mojom::KeyringId::kZCashMainnet,
                                          mojom::AccountKind::kDerived, 0);
    auto lambda = base::BindLambdaForTesting(
        [&](base::expected<OrchardStorage::Result, OrchardStorage::Error>
                result) {
          EXPECT_EQ(OrchardStorage::Result::kSuccess, result.value());
        });
    sync_state_.AsyncCall(&OrchardSyncState::RegisterAccount)
        .WithArgs(account_id_.Clone(), kNu5BlockUpdate + 1)
        .Then(std::move(lambda));

    InitZCashRpc();
  }

  void TearDown() override { sync_state_.SynchronouslyResetForTest(); }

  void InitZCashRpc() {
    ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
        .WillByDefault([](const std::string& chain_id,
                          zcash::mojom::BlockIDPtr block,
                          ZCashRpc::GetTreeStateCallback callback) {
          // Valid tree state
          auto tree_state = zcash::mojom::TreeState::New(
              chain_id, block->height, "aabb", 0, "", "", "");
          std::move(callback).Run(std::move(tree_state));
        });

    ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
        .WillByDefault([](const std::string& chain_id, uint32_t from,
                          uint32_t to,
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
        });
  }

  ZCashActionContext CreateContext() { return CreateContext(account_id_); }

  ZCashActionContext CreateContext(const mojom::AccountIdPtr& account_id) {
    return ZCashActionContext(zcash_rpc_, {}, sync_state_, account_id);
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                 OrchardStorage::Error>
  GetSpendableNotes() {
    std::optional<
        base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                       OrchardStorage::Error>>
        result;
    sync_state_.AsyncCall(&OrchardSyncState::GetSpendableNotes)
        .WithArgs(OrchardPool::kOrchard, account_id_.Clone(),
                  OrchardAddrRawPart({}))
        .Then(base::BindLambdaForTesting(
            [&](base::expected<
                std::optional<OrchardSyncState::SpendableNotesBundle>,
                OrchardStorage::Error> r) { result = std::move(r); }));
    task_environment().RunUntilIdle();
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
              blocks.back()->height, ToHex(blocks.back()->hash));
          for (const auto& block : blocks) {
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

            if (block->height == kNu5BlockUpdate + 255) {
              result.orchard.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 1)}));
            } else if (block->height == kNu5BlockUpdate + 265) {
              result.orchard.found_spends.push_back(OrchardNoteSpend(
                  block->height, {GenerateMockNullifier(account_id, 2)}));
            }

            if (block->height == kNu5BlockUpdate + 405) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 4));
            } else if (block->height == kNu5BlockUpdate + 505) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 5));
            }
          }
          std::move(callback).Run(std::move(result));
        }));
  }

  // Simulates a scan batch straddling the Ironwood activation height on
  // testnet: orchard notes are discovered before activation, while ironwood
  // notes and both pools' nullifiers only appear after it. Mirrors real
  // scanning behavior in that ironwood data is only produced when
  // `ironwood_tree_state` is present, i.e. when the Ironwood feature is
  // enabled and the batch reaches the activation height.
  std::unique_ptr<MockOrchardBlockScannerProxy>
  CreateIronwoodActivationBlockScannerProxy() {
    return std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
        [](OrchardTreeState tree_state,
           std::optional<OrchardTreeState> ironwood_tree_state,
           std::vector<zcash::mojom::CompactBlockPtr> blocks,
           base::OnceCallback<void(
               base::expected<OrchardBlockScanner::Result,
                              OrchardBlockScanner::ErrorCode>)> callback) {
          auto account_id = MakeIndexBasedAccountId(
              mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
              mojom::AccountKind::kDerived, 0);
          constexpr uint32_t kActivation = kIronwoodActivationHeightTestnet;

          OrchardBlockScanner::Result result = CreateResultForTesting(
              std::move(tree_state), std::vector<OrchardCommitment>(),
              blocks.back()->height, ToHex(blocks.back()->hash));
          if (ironwood_tree_state) {
            result.ironwood = CreateIronwoodPoolResultForTesting(
                std::move(*ironwood_tree_state),
                std::vector<OrchardCommitment>(), blocks.back()->height,
                ToHex(blocks.back()->hash));
          }

          for (const auto& block : blocks) {
            // Orchard notes discovered before Ironwood activation.
            if (block->height == kActivation - 15) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 1));
            } else if (block->height == kActivation - 5) {
              result.orchard.discovered_notes.push_back(
                  GenerateMockOrchardNote(account_id, block->height, 2));
            } else if (block->height == kActivation + 10) {
              // Orchard nullifier found after activation, spending note 1.
              result.orchard.found_spends.push_back(
                  GenerateMockNoteSpend(account_id, block->height, 1));
            }

            if (result.ironwood) {
              if (block->height == kActivation + 5) {
                result.ironwood->discovered_notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 11));
              } else if (block->height == kActivation + 8) {
                result.ironwood->discovered_notes.push_back(
                    GenerateMockOrchardNote(account_id, block->height, 12));
              } else if (block->height == kActivation + 15) {
                // Ironwood nullifier found after activation, spending
                // note 11.
                result.ironwood->found_spends.push_back(
                    GenerateMockNoteSpend(account_id, block->height, 11));
              }
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

TEST_F(ZCashBlocksBatchScanTest, SingleBlockDecoded) {
  ZCashActionContext context = CreateContext();

  std::vector<uint32_t> decoded_blocks;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::vector<uint32_t>* decoded_blocks, OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks.back()->height, ToHex(blocks.back()->hash));
            for (const auto& block : blocks) {
              decoded_blocks->push_back(block->height);
            }
            std::move(callback).Run(std::move(result));
          },
          &decoded_blocks));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 1},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_TRUE(result_future.Get().has_value());
  EXPECT_EQ(decoded_blocks.size(), 1u);
  EXPECT_EQ(decoded_blocks[0], kNu5BlockUpdate + 1);
}

TEST_F(ZCashBlocksBatchScanTest, AllBlocksDecoded) {
  ZCashActionContext context = CreateContext();

  std::vector<uint32_t> decoded_blocks;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::vector<uint32_t>* decoded_blocks, OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks.back()->height, ToHex(blocks.back()->hash));
            for (const auto& block : blocks) {
              decoded_blocks->push_back(block->height);
            }
            std::move(callback).Run(std::move(result));
          },
          &decoded_blocks));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 400},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_TRUE(result_future.Get().has_value());
  // We shouldn't have any notes added since one block is corrupted
  EXPECT_EQ(decoded_blocks.size(), 400u);
  for (int i = 0; i < 400; i++) {
    EXPECT_EQ(decoded_blocks[i], i + 1 + kNu5BlockUpdate);
  }
}

TEST_F(ZCashBlocksBatchScanTest, Scan) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 500},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_TRUE(result_future.Get().has_value());
  auto value = task.TakeResult();
  EXPECT_EQ(value.orchard.discovered_notes.size(), 4u);
  EXPECT_EQ(value.orchard.found_spends.size(), 2u);
}

TEST_F(ZCashBlocksBatchScanTest, Error_PartialScan) {
  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 700},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_FALSE(result_future.Get().has_value());
}

TEST_F(ZCashBlocksBatchScanTest, Error_PartialDecoding) {
  ZCashActionContext context = CreateContext();

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
                blocks.back()->height, ToHex(blocks.back()->hash));
            for (const auto& block : blocks) {
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

              if (block->height == kNu5BlockUpdate + 355) {
                std::move(callback).Run(base::unexpected(
                    OrchardBlockScanner::ErrorCode::kDecoderError));
                return;
              }
            }
            std::move(callback).Run(std::move(result));
          }));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 400},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_FALSE(result_future.Get().has_value());
}

TEST_F(ZCashBlocksBatchScanTest, NetworkError_Blocks) {
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 200},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_FALSE(result_future.Get().has_value());
}

TEST_F(ZCashBlocksBatchScanTest, NetworkError_TreeState) {
  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  auto block_scanner = CreateMockOrchardBlockScannerProxy();
  ZCashActionContext context = CreateContext();

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 200},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_FALSE(result_future.Get().has_value());
}

TEST_F(ZCashBlocksBatchScanTest, DecodingError) {
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

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 200},
                                       result_future.GetCallback());
  task.Start();

  EXPECT_FALSE(result_future.Get().has_value());
}

// Flag ON, but mainnet scan range is still below activation: the scanner
// must receive std::nullopt for the ironwood tree state.
TEST_F(ZCashBlocksBatchScanTest, IronwoodInactiveOnMainnet) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  ZCashActionContext context = CreateContext();  // mainnet

  std::optional<bool> ironwood_present;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::optional<bool>* out, OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            *out = ironwood_tree_state.has_value();
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks.back()->height, ToHex(blocks.back()->hash));
            std::move(callback).Run(std::move(result));
          },
          &ironwood_present));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kNu5BlockUpdate + 1, 1},
                                       result_future.GetCallback());
  task.Start();
  EXPECT_TRUE(result_future.Wait());

  ASSERT_TRUE(ironwood_present.has_value());
  EXPECT_FALSE(ironwood_present.value());
}

// Flag ON and testnet block at/above the activation height: the scanner must
// receive a populated ironwood tree state.
TEST_F(ZCashBlocksBatchScanTest, IronwoodActiveOnTestnet) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  // Default mock caps blocks at kNu5BlockUpdate + 600; serve any height here.
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        std::vector<zcash::mojom::CompactBlockPtr> blocks;
        for (uint32_t i = from; i <= to; i++) {
          auto chain_metadata = zcash::mojom::ChainMetadata::New();
          chain_metadata->orchard_commitment_tree_size = 0;
          chain_metadata->ironwood_commitment_tree_size = 0;
          blocks.push_back(zcash::mojom::CompactBlock::New(
              0u, i, std::vector<uint8_t>({0xbb, 0xaa}), std::vector<uint8_t>(),
              0u, std::vector<uint8_t>(),
              std::vector<zcash::mojom::CompactTxPtr>(),
              std::move(chain_metadata)));
        }
        std::move(callback).Run(std::move(blocks));
      });

  auto testnet_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
      mojom::AccountKind::kDerived, 0);
  ZCashActionContext context = CreateContext(testnet_account_id);

  std::optional<bool> ironwood_present;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::optional<bool>* out, OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            *out = ironwood_tree_state.has_value();
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks.back()->height, ToHex(blocks.back()->hash));
            std::move(callback).Run(std::move(result));
          },
          &ironwood_present));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(context, *block_scanner,
                                       {kIronwoodActivationHeightTestnet, 1},
                                       result_future.GetCallback());
  task.Start();
  EXPECT_TRUE(result_future.Wait());

  ASSERT_TRUE(ironwood_present.has_value());
  EXPECT_TRUE(ironwood_present.value());
}

// Flag ON and a scan range straddling the Ironwood activation height: the
// frontier is pre-activation, so the scanner must receive an empty Ironwood
// prior tree state (tree_size 0, empty frontier).
TEST_F(ZCashBlocksBatchScanTest, IronwoodStraddlingBatchEmptyTreeState) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  // Default mock caps blocks at kNu5BlockUpdate + 600; serve any height here.
  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        std::vector<zcash::mojom::CompactBlockPtr> blocks;
        for (uint32_t i = from; i <= to; i++) {
          auto chain_metadata = zcash::mojom::ChainMetadata::New();
          chain_metadata->orchard_commitment_tree_size = 0;
          chain_metadata->ironwood_commitment_tree_size = 0;
          blocks.push_back(zcash::mojom::CompactBlock::New(
              0u, i, std::vector<uint8_t>({0xbb, 0xaa}), std::vector<uint8_t>(),
              0u, std::vector<uint8_t>(),
              std::vector<zcash::mojom::CompactTxPtr>(),
              std::move(chain_metadata)));
        }
        std::move(callback).Run(std::move(blocks));
      });

  auto testnet_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
      mojom::AccountKind::kDerived, 0);
  ZCashActionContext context = CreateContext(testnet_account_id);

  std::optional<OrchardTreeState> captured_ironwood;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::optional<OrchardTreeState>* out, OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            *out = std::move(ironwood_tree_state);
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks.back()->height, ToHex(blocks.back()->hash));
            std::move(callback).Run(std::move(result));
          },
          &captured_ironwood));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  // Frontier = activation - 2 (pre-activation); last block = activation + 1
  // (Ironwood decode enabled).
  auto task = ZCashBlocksBatchScanTask(
      context, *block_scanner, {kIronwoodActivationHeightTestnet - 1, 3},
      result_future.GetCallback());
  task.Start();
  EXPECT_TRUE(result_future.Wait());

  ASSERT_TRUE(captured_ironwood.has_value());
  EXPECT_EQ(captured_ironwood->tree_size, 0u);
  EXPECT_TRUE(captured_ironwood->frontier.empty());
}

// Flag ON and the frontier block at/after the activation height with a
// non-empty ironwoodTree hex string: the scanner must receive the decoded
// frontier bytes and the real tree size from the frontier block's chain
// metadata (mirrors the orchard frontier decoding path).
TEST_F(ZCashBlocksBatchScanTest, IronwoodFrontierDecoded) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            chain_id, block->height, "aabb", 0, "", "", "aabb");
        std::move(callback).Run(std::move(tree_state));
      });

  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        std::vector<zcash::mojom::CompactBlockPtr> blocks;
        for (uint32_t i = from; i <= to; i++) {
          auto chain_metadata = zcash::mojom::ChainMetadata::New();
          chain_metadata->orchard_commitment_tree_size = 0;
          chain_metadata->ironwood_commitment_tree_size = 5;
          blocks.push_back(zcash::mojom::CompactBlock::New(
              0u, i, std::vector<uint8_t>({0xbb, 0xaa}), std::vector<uint8_t>(),
              0u, std::vector<uint8_t>(),
              std::vector<zcash::mojom::CompactTxPtr>(),
              std::move(chain_metadata)));
        }
        std::move(callback).Run(std::move(blocks));
      });

  auto testnet_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
      mojom::AccountKind::kDerived, 0);
  ZCashActionContext context = CreateContext(testnet_account_id);

  std::optional<OrchardTreeState> captured_ironwood;
  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](std::optional<OrchardTreeState>* out, OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            *out = std::move(ironwood_tree_state);
            OrchardBlockScanner::Result result = CreateResultForTesting(
                std::move(tree_state), std::vector<OrchardCommitment>(),
                blocks.back()->height, ToHex(blocks.back()->hash));
            std::move(callback).Run(std::move(result));
          },
          &captured_ironwood));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  // Frontier block height == activation height, so the "post-activation"
  // branch (real tree size + decoded frontier) must be taken.
  auto task = ZCashBlocksBatchScanTask(
      context, *block_scanner, {kIronwoodActivationHeightTestnet + 1, 1},
      result_future.GetCallback());
  task.Start();
  EXPECT_TRUE(result_future.Wait());

  ASSERT_TRUE(captured_ironwood.has_value());
  EXPECT_EQ(captured_ironwood->tree_size, 5u);
  EXPECT_THAT(captured_ironwood->frontier, testing::ElementsAre(0xaa, 0xbb));
}

// Flag ON and the frontier tree state's ironwoodTree is not valid hex: the
// task must fail with kScannerError and never hand control to the scanner.
TEST_F(ZCashBlocksBatchScanTest, IronwoodFrontierInvalidHexError) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  ON_CALL(zcash_rpc(), GetTreeState(_, _, _))
      .WillByDefault([](const std::string& chain_id,
                        zcash::mojom::BlockIDPtr block,
                        ZCashRpc::GetTreeStateCallback callback) {
        auto tree_state = zcash::mojom::TreeState::New(
            chain_id, block->height, "aabb", 0, "", "", "not_hex");
        std::move(callback).Run(std::move(tree_state));
      });

  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        std::vector<zcash::mojom::CompactBlockPtr> blocks;
        for (uint32_t i = from; i <= to; i++) {
          auto chain_metadata = zcash::mojom::ChainMetadata::New();
          chain_metadata->orchard_commitment_tree_size = 0;
          chain_metadata->ironwood_commitment_tree_size = 0;
          blocks.push_back(zcash::mojom::CompactBlock::New(
              0u, i, std::vector<uint8_t>({0xbb, 0xaa}), std::vector<uint8_t>(),
              0u, std::vector<uint8_t>(),
              std::vector<zcash::mojom::CompactTxPtr>(),
              std::move(chain_metadata)));
        }
        std::move(callback).Run(std::move(blocks));
      });

  auto testnet_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
      mojom::AccountKind::kDerived, 0);
  ZCashActionContext context = CreateContext(testnet_account_id);

  auto block_scanner =
      std::make_unique<MockOrchardBlockScannerProxy>(base::BindRepeating(
          [](OrchardTreeState tree_state,
             std::optional<OrchardTreeState> ironwood_tree_state,
             std::vector<zcash::mojom::CompactBlockPtr> blocks,
             base::OnceCallback<void(
                 base::expected<OrchardBlockScanner::Result,
                                OrchardBlockScanner::ErrorCode>)> callback) {
            ADD_FAILURE() << "Scanner should not run when the ironwood "
                             "frontier hex fails to parse";
          }));

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(
      context, *block_scanner, {kIronwoodActivationHeightTestnet + 1, 1},
      result_future.GetCallback());
  task.Start();

  auto result = result_future.Get();
  EXPECT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code,
            ZCashShieldSyncService::ErrorCode::kScannerError);
}

// Flag ON and a batch straddling the Ironwood activation height: orchard
// notes discovered before activation, plus orchard and ironwood nullifiers
// and ironwood notes discovered after it, must all end up in the result.
TEST_F(ZCashBlocksBatchScanTest, IronwoodActivation_Enabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "true"}});

  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        std::vector<zcash::mojom::CompactBlockPtr> blocks;
        for (uint32_t i = from; i <= to; i++) {
          auto chain_metadata = zcash::mojom::ChainMetadata::New();
          chain_metadata->orchard_commitment_tree_size = 0;
          chain_metadata->ironwood_commitment_tree_size = 0;
          blocks.push_back(zcash::mojom::CompactBlock::New(
              0u, i, std::vector<uint8_t>({0xbb, 0xaa}), std::vector<uint8_t>(),
              0u, std::vector<uint8_t>(),
              std::vector<zcash::mojom::CompactTxPtr>(),
              std::move(chain_metadata)));
        }
        std::move(callback).Run(std::move(blocks));
      });

  auto testnet_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
      mojom::AccountKind::kDerived, 0);
  ZCashActionContext context = CreateContext(testnet_account_id);

  auto block_scanner = CreateIronwoodActivationBlockScannerProxy();

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(
      context, *block_scanner, {kIronwoodActivationHeightTestnet - 20, 41},
      result_future.GetCallback());
  task.Start();

  EXPECT_TRUE(result_future.Get().has_value());
  auto value = task.TakeResult();
  EXPECT_EQ(value.orchard.discovered_notes.size(), 2u);
  EXPECT_EQ(value.orchard.found_spends.size(), 1u);
  ASSERT_TRUE(value.ironwood.has_value());
  EXPECT_EQ(value.ironwood->discovered_notes.size(), 2u);
  EXPECT_EQ(value.ironwood->found_spends.size(), 1u);
}

// Flag OFF and the same batch straddling the Ironwood activation height:
// orchard notes/nullifiers are unaffected, but no ironwood tree state is
// ever passed to the scanner, so the result must have no ironwood data.
TEST_F(ZCashBlocksBatchScanTest, IronwoodActivation_Disabled) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"},
       {"zcash_ironwood_enabled", "false"}});

  ON_CALL(zcash_rpc(), GetCompactBlocks(_, _, _, _))
      .WillByDefault([](const std::string& chain_id, uint32_t from, uint32_t to,
                        ZCashRpc::GetCompactBlocksCallback callback) {
        std::vector<zcash::mojom::CompactBlockPtr> blocks;
        for (uint32_t i = from; i <= to; i++) {
          auto chain_metadata = zcash::mojom::ChainMetadata::New();
          chain_metadata->orchard_commitment_tree_size = 0;
          chain_metadata->ironwood_commitment_tree_size = 0;
          blocks.push_back(zcash::mojom::CompactBlock::New(
              0u, i, std::vector<uint8_t>({0xbb, 0xaa}), std::vector<uint8_t>(),
              0u, std::vector<uint8_t>(),
              std::vector<zcash::mojom::CompactTxPtr>(),
              std::move(chain_metadata)));
        }
        std::move(callback).Run(std::move(blocks));
      });

  auto testnet_account_id = MakeIndexBasedAccountId(
      mojom::CoinType::ZEC, mojom::KeyringId::kZCashTestnet,
      mojom::AccountKind::kDerived, 0);
  ZCashActionContext context = CreateContext(testnet_account_id);

  auto block_scanner = CreateIronwoodActivationBlockScannerProxy();

  base::test::TestFuture<base::expected<void, ZCashShieldSyncService::Error>>
      result_future;
  auto task = ZCashBlocksBatchScanTask(
      context, *block_scanner, {kIronwoodActivationHeightTestnet - 20, 41},
      result_future.GetCallback());
  task.Start();

  EXPECT_TRUE(result_future.Get().has_value());
  auto value = task.TakeResult();
  EXPECT_EQ(value.orchard.discovered_notes.size(), 2u);
  EXPECT_EQ(value.orchard.found_spends.size(), 1u);
  EXPECT_FALSE(value.ironwood.has_value());
}

}  // namespace brave_wallet
