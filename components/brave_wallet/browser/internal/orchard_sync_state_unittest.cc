/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"

#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_types.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_testing_shard_tree.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr uint32_t kDefaultCommitmentSeed = 1;

OrchardNoteWitness CreateWitness(const std::vector<std::string>& path,
                                 uint32_t position) {
  OrchardNoteWitness result;
  for (const auto& path_elem : path) {
    OrchardMerkleHash as_bytes;
    EXPECT_TRUE(base::HexStringToSpan(path_elem, as_bytes));
    result.merkle_path.push_back(as_bytes);
  }
  result.position = position;
  return result;
}

OrchardCommitment CreateCommitment(OrchardCommitmentValue value,
                                   bool marked,
                                   std::optional<uint32_t> checkpoint_id) {
  return OrchardCommitment{std::move(value), marked, checkpoint_id};
}

}  // namespace

class OrchardSyncStateTest : public testing::Test {
 public:
  OrchardSyncStateTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override;

  OrchardSyncState* sync_state() { return sync_state_.get(); }

  OrchardStorage& storage() { return sync_state_->orchard_storage(); }

  mojom::AccountIdPtr account_id() { return account_id_.Clone(); }

 private:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<OrchardSyncState> sync_state_;
};

void OrchardSyncStateTest::SetUp() {
  account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                        mojom::KeyringId::kZCashMainnet,
                                        mojom::AccountKind::kDerived, 0);
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
  sync_state_ = std::make_unique<OrchardSyncState>(db_path);
  sync_state_->OverrideShardTreeForTesting(
      account_id_, orchard::CreateShardTreeForTesting(
                       sync_state_->orchard_storage(), account_id_));
}

TEST_F(OrchardSyncStateTest, CheckpointsPruned) {
  std::vector<OrchardCommitment> commitments;

  for (int i = 0; i < 40; i++) {
    std::optional<uint32_t> checkpoint;
    if (i % 2 == 0) {
      checkpoint = i * 2;
    }

    commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(i, kDefaultCommitmentSeed),
                         false, checkpoint));
  }
  OrchardTreeState orchard_tree_state;
  auto result = CreateResultForTesting(std::move(orchard_tree_state),
                                       std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  EXPECT_EQ(10u, storage().CheckpointCount(account_id()).value());
  EXPECT_EQ(40u, storage().MinCheckpointId(account_id()).value().value());
  EXPECT_EQ(76u, storage().MaxCheckpointId(account_id()).value().value());
}

TEST_F(OrchardSyncStateTest, InsertWithFrontier) {
  OrchardTreeState prior_tree_state;
  prior_tree_state.block_height = 0;
  prior_tree_state.tree_size = 48;
  prior_tree_state.frontier = std::vector<uint8_t>(
      {1,   72,  173, 200, 225, 47,  142, 44,  148, 137, 119, 18,  99,  211,
       92,  65,  67,  173, 197, 93,  7,   85,  70,  105, 140, 223, 184, 193,
       172, 9,   194, 88,  62,  1,   130, 31,  76,  59,  69,  55,  151, 124,
       101, 120, 230, 247, 201, 82,  48,  160, 150, 48,  23,  84,  250, 117,
       120, 175, 108, 220, 96,  214, 42,  255, 209, 44,  7,   1,   13,  59,
       69,  136, 45,  180, 148, 18,  146, 125, 241, 196, 224, 205, 11,  196,
       195, 90,  164, 186, 175, 22,  90,  105, 82,  149, 34,  131, 232, 132,
       223, 15,  1,   211, 200, 193, 46,  24,  11,  42,  42,  182, 124, 29,
       48,  234, 215, 28,  103, 218, 239, 234, 109, 10,  231, 74,  70,  197,
       113, 131, 89,  199, 71,  102, 33,  1,   153, 86,  62,  213, 2,   98,
       191, 65,  218, 123, 73,  155, 243, 225, 45,  10,  241, 132, 49,  33,
       101, 183, 59,  35,  56,  78,  228, 47,  166, 10,  237, 50,  0,   1,
       94,  228, 186, 123, 0,   136, 39,  192, 226, 129, 40,  253, 0,   83,
       248, 138, 7,   26,  120, 212, 191, 135, 44,  0,   171, 42,  69,  6,
       133, 205, 115, 4,   0,   0});

  std::vector<OrchardCommitment> commitments;

  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(48, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(49, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(50, kDefaultCommitmentSeed),
                       true, std::nullopt));
  commitments.push_back(CreateCommitment(
      CreateMockCommitmentValue(51, kDefaultCommitmentSeed), false, 1));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(52, kDefaultCommitmentSeed),
                       false, std::nullopt));

  auto result = CreateResultForTesting(std::move(prior_tree_state),
                                       std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 50;

  auto witness_result =
      sync_state()->CalculateWitnessForCheckpoint(account_id(), {input}, 1);
  EXPECT_TRUE(witness_result.has_value());
  EXPECT_EQ(
      witness_result.value()[0].witness.value(),
      CreateWitness(
          {"9695d64b1ccd38aa5dfdc5c70aecf0e763549034318c59943a3e3e921b415c3a",
           "48ddf8a84afc5949e074c162630e3f6aab3d4350bf929ba82677cee4c634e029",
           "c7413f4614cd64043abbab7cc1095c9bb104231cea89e2c3e0df83769556d030",
           "2111fc397753e5fd50ec74816df27d6ada7ed2a9ac3816aab2573c8fac794204",
           "2d99471d096691e4a5f43efe469734aff37f4f21c707b060c952a84169f9302f",
           "5ee4ba7b008827c0e28128fd0053f88a071a78d4bf872c00ab2a450685cd7304",
           "27ab1320953ae1ad70c8c15a1253a0a86fbc8a0aa36a84207293f8a495ffc402",
           "4e14563df191a2a65b4b37113b5230680555051b22d74a8e1f1d706f90f3133"
           "b"},
          50));
}

TEST_F(OrchardSyncStateTest, Checkpoint_WithMarked) {
  std::vector<OrchardCommitment> commitments;

  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(2, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(CreateCommitment(
      CreateMockCommitmentValue(3, kDefaultCommitmentSeed), true, 1));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(4, kDefaultCommitmentSeed),
                       false, std::nullopt));

  auto result =
      CreateResultForTesting(OrchardTreeState(), std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 3;
  auto witness_result =
      sync_state()->CalculateWitnessForCheckpoint(account_id(), {input}, 1);
  EXPECT_TRUE(witness_result.has_value());

  EXPECT_EQ(
      witness_result.value()[0].witness.value(),
      CreateWitness(
          {"3bb11bd05d2ed5e590369f274a1a247d390380aa0590160bfbf72cb186d7023f",
           "d4059d13ddcbe9ec7e6fc99bdf9bfd08b0a678d26e3bf6a734e7688eca669f37",
           "c7413f4614cd64043abbab7cc1095c9bb104231cea89e2c3e0df83769556d030",
           "2111fc397753e5fd50ec74816df27d6ada7ed2a9ac3816aab2573c8fac794204",
           "806afbfeb45c64d4f2384c51eff30764b84599ae56a7ab3d4a46d9ce3aeab431",
           "873e4157f2c0f0c645e899360069fcc9d2ed9bc11bf59827af0230ed52edab18",
           "27ab1320953ae1ad70c8c15a1253a0a86fbc8a0aa36a84207293f8a495ffc402",
           "4e14563df191a2a65b4b37113b5230680555051b22d74a8e1f1d706f90f3133"
           "b"},
          3));
}

TEST_F(OrchardSyncStateTest, MinCheckpoint) {
  std::vector<OrchardCommitment> commitments;

  for (int i = 0; i < 40; i++) {
    std::optional<uint32_t> checkpoint;
    if (i % 2 == 0) {
      checkpoint = i * 2;
    }
    commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(i, kDefaultCommitmentSeed),
                         false, checkpoint));
  }
  auto result =
      CreateResultForTesting(OrchardTreeState(), std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  EXPECT_EQ(10u, storage().CheckpointCount(account_id()).value());
  EXPECT_EQ(40u, storage().MinCheckpointId(account_id()).value().value());
  EXPECT_EQ(76u, storage().MaxCheckpointId(account_id()).value().value());
}

TEST_F(OrchardSyncStateTest, MaxCheckpoint) {
  {
    std::vector<OrchardCommitment> commitments;

    for (int i = 0; i < 5; i++) {
      commitments.push_back(
          CreateCommitment(CreateMockCommitmentValue(i, kDefaultCommitmentSeed),
                           false, std::nullopt));
    }
    commitments.push_back(CreateCommitment(
        CreateMockCommitmentValue(5, kDefaultCommitmentSeed), false, 1u));
    auto result =
        CreateResultForTesting(OrchardTreeState(), std::move(commitments));
    EXPECT_EQ(
        OrchardStorage::Result::kSuccess,
        sync_state()
            ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
            .value());
  }

  {
    std::vector<OrchardCommitment> commitments;

    for (int i = 6; i < 10; i++) {
      commitments.push_back(
          CreateCommitment(CreateMockCommitmentValue(i, kDefaultCommitmentSeed),
                           false, std::nullopt));
    }
    commitments.push_back(CreateCommitment(
        CreateMockCommitmentValue(10, kDefaultCommitmentSeed), false, 2u));
    OrchardTreeState tree_state;
    tree_state.block_height = 1;
    tree_state.tree_size = 6;
    auto result =
        CreateResultForTesting(std::move(tree_state), std::move(commitments));
    EXPECT_EQ(
        OrchardStorage::Result::kSuccess,
        sync_state()
            ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
            .value());
  }

  {
    std::vector<OrchardCommitment> commitments;

    for (int i = 11; i < 15; i++) {
      commitments.push_back(
          CreateCommitment(CreateMockCommitmentValue(i, kDefaultCommitmentSeed),
                           false, std::nullopt));
    }
    commitments.push_back(CreateCommitment(
        CreateMockCommitmentValue(15, kDefaultCommitmentSeed), false, 3u));
    OrchardTreeState tree_state;
    tree_state.block_height = 2;
    tree_state.tree_size = 11;
    auto result =
        CreateResultForTesting(std::move(tree_state), std::move(commitments));
    EXPECT_EQ(
        OrchardStorage::Result::kSuccess,
        sync_state()
            ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
            .value());
  }

  EXPECT_EQ(3u, storage().CheckpointCount(account_id()).value());
  EXPECT_EQ(1u, storage().MinCheckpointId(account_id()).value().value());
  EXPECT_EQ(3u, storage().MaxCheckpointId(account_id()).value().value());
}

TEST_F(OrchardSyncStateTest, NoWitnessOnNonMarked) {
  std::vector<OrchardCommitment> commitments;

  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(2, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(CreateCommitment(
      CreateMockCommitmentValue(3, kDefaultCommitmentSeed), false, 1));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(4, kDefaultCommitmentSeed),
                       false, std::nullopt));

  auto result =
      CreateResultForTesting(OrchardTreeState(), std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  {
    OrchardInput input;
    input.note.orchard_commitment_tree_position = 2;
    auto witness_result =
        sync_state()->CalculateWitnessForCheckpoint(account_id(), {input}, 1);
    EXPECT_FALSE(witness_result.has_value());
  }
}

TEST_F(OrchardSyncStateTest, NoWitnessOnWrongCheckpoint) {
  std::vector<OrchardCommitment> commitments;

  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(2, kDefaultCommitmentSeed),
                       true, std::nullopt));
  commitments.push_back(CreateCommitment(
      CreateMockCommitmentValue(3, kDefaultCommitmentSeed), false, 1));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(4, kDefaultCommitmentSeed),
                       false, std::nullopt));

  auto result =
      CreateResultForTesting(OrchardTreeState(), std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  {
    OrchardInput input;
    input.note.orchard_commitment_tree_position = 2;
    auto witness_result =
        sync_state()->CalculateWitnessForCheckpoint(account_id(), {input}, 2);
    EXPECT_FALSE(witness_result.has_value());
  }
}

TEST_F(OrchardSyncStateTest, TruncateTree) {
  {
    std::vector<OrchardCommitment> commitments;

    for (int i = 0; i < 10; i++) {
      switch (i) {
        case 2:
          commitments.push_back(CreateCommitment(
              CreateMockCommitmentValue(i, kDefaultCommitmentSeed), true,
              std::nullopt));
          break;
        case 3:
          commitments.push_back(CreateCommitment(
              CreateMockCommitmentValue(i, kDefaultCommitmentSeed), false, 1));
          break;
        case 5:
          commitments.push_back(CreateCommitment(
              CreateMockCommitmentValue(i, kDefaultCommitmentSeed), false, 2));
          break;
        default:
          commitments.push_back(CreateCommitment(
              CreateMockCommitmentValue(i, kDefaultCommitmentSeed), false,
              std::nullopt));
          break;
      }
    }

    auto result =
        CreateResultForTesting(OrchardTreeState(), std::move(commitments));
    EXPECT_EQ(
        OrchardStorage::Result::kSuccess,
        sync_state()
            ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
            .value());
  }

  EXPECT_TRUE(sync_state()->Truncate(account_id(), 2));

  {
    std::vector<OrchardCommitment> commitments;

    for (int j = 0; j < 5; j++) {
      if (j == 3) {
        commitments.push_back(
            CreateCommitment(CreateMockCommitmentValue(j, 5), false, 2));
      } else {
        commitments.push_back(CreateCommitment(CreateMockCommitmentValue(j, 5),
                                               false, std::nullopt));
      }
    }

    OrchardTreeState tree_state;
    tree_state.block_height = 1;
    // Truncate was on position 5, so 5 elements left in the tre
    tree_state.tree_size = 5;
    auto result =
        CreateResultForTesting(std::move(tree_state), std::move(commitments));
    EXPECT_EQ(
        OrchardStorage::Result::kSuccess,
        sync_state()
            ->ApplyScanResults(account_id(), std::move(result), 2000, "2000")
            .value());
  }

  {
    OrchardInput input;
    input.note.orchard_commitment_tree_position = 2;
    auto witness_result =
        sync_state()->CalculateWitnessForCheckpoint(account_id(), {input}, 2);
    EXPECT_TRUE(witness_result.has_value());
  }

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 2;
  auto witness_result =
      sync_state()->CalculateWitnessForCheckpoint(account_id(), {input}, 1);
  EXPECT_TRUE(witness_result.has_value());
  EXPECT_EQ(
      witness_result.value()[0].witness.value(),
      CreateWitness(
          {"f342eb6489f4e5b5a0fb0a4ece48d137dcd5e80011aab4668913f98be2af3311",
           "d4059d13ddcbe9ec7e6fc99bdf9bfd08b0a678d26e3bf6a734e7688eca669f37",
           "c7413f4614cd64043abbab7cc1095c9bb104231cea89e2c3e0df83769556d030",
           "2111fc397753e5fd50ec74816df27d6ada7ed2a9ac3816aab2573c8fac794204",
           "806afbfeb45c64d4f2384c51eff30764b84599ae56a7ab3d4a46d9ce3aeab431",
           "873e4157f2c0f0c645e899360069fcc9d2ed9bc11bf59827af0230ed52edab18",
           "27ab1320953ae1ad70c8c15a1253a0a86fbc8a0aa36a84207293f8a495ffc402",
           "4e14563df191a2a65b4b37113b5230680555051b22d74a8e1f1d706f90f3133"
           "b"},
          2));
}

TEST_F(OrchardSyncStateTest, TruncateTreeWrongCheckpoint) {
  std::vector<OrchardCommitment> commitments;

  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(2, kDefaultCommitmentSeed),
                       true, std::nullopt));
  commitments.push_back(CreateCommitment(
      CreateMockCommitmentValue(3, kDefaultCommitmentSeed), false, 1));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(4, kDefaultCommitmentSeed),
                       false, std::nullopt));

  auto result =
      CreateResultForTesting(OrchardTreeState(), std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  EXPECT_FALSE(sync_state()->Truncate(account_id(), 2));
}

TEST_F(OrchardSyncStateTest, SimpleInsert) {
  std::vector<OrchardCommitment> commitments;

  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(0, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(1, kDefaultCommitmentSeed),
                       false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(2, kDefaultCommitmentSeed),
                       true, std::nullopt));
  commitments.push_back(CreateCommitment(
      CreateMockCommitmentValue(3, kDefaultCommitmentSeed), false, 1));
  commitments.push_back(
      CreateCommitment(CreateMockCommitmentValue(4, kDefaultCommitmentSeed),
                       false, std::nullopt));

  auto result =
      CreateResultForTesting(OrchardTreeState(), std::move(commitments));
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()
          ->ApplyScanResults(account_id(), std::move(result), 1000, "1000")
          .value());

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 2;
  auto witness_result =
      sync_state()->CalculateWitnessForCheckpoint(account_id(), {input}, 1);
  EXPECT_TRUE(witness_result.has_value());
  EXPECT_EQ(
      witness_result.value()[0].witness.value(),
      CreateWitness(
          {"f342eb6489f4e5b5a0fb0a4ece48d137dcd5e80011aab4668913f98be2af3311",
           "d4059d13ddcbe9ec7e6fc99bdf9bfd08b0a678d26e3bf6a734e7688eca669f37",
           "c7413f4614cd64043abbab7cc1095c9bb104231cea89e2c3e0df83769556d030",
           "2111fc397753e5fd50ec74816df27d6ada7ed2a9ac3816aab2573c8fac794204",
           "806afbfeb45c64d4f2384c51eff30764b84599ae56a7ab3d4a46d9ce3aeab431",
           "873e4157f2c0f0c645e899360069fcc9d2ed9bc11bf59827af0230ed52edab18",
           "27ab1320953ae1ad70c8c15a1253a0a86fbc8a0aa36a84207293f8a495ffc402",
           "4e14563df191a2a65b4b37113b5230680555051b22d74a8e1f1d706f90f3133"
           "b"},
          2));
}

}  // namespace brave_wallet
