/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"

#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_types.h"
#include "brave/components/brave_wallet/browser/internal/orchard_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_testing_shard_tree.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
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

}  // namespace

class OrchardSyncStateTest : public testing::Test {
 public:
  OrchardSyncStateTest() = default;
  void SetUp() override;

  OrchardSyncState* sync_state() { return sync_state_.get(); }

  OrchardStorage& storage() { return sync_state_->orchard_storage(); }

  mojom::AccountIdPtr account_id() { return account_id_.Clone(); }

 private:
  base::ScopedTempDir temp_dir_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<OrchardSyncState> sync_state_;
};

void OrchardSyncStateTest::SetUp() {
  account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                        mojom::KeyringId::kZCashMainnet,
                                        mojom::AccountKind::kDerived, 0);
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  sync_state_ = std::make_unique<OrchardSyncState>(temp_dir_.GetPath());
  sync_state_->OverrideShardTreeForTesting(
      OrchardPool::kOrchard, account_id_,
      orchard::CreateShardTreeForTesting(sync_state_->orchard_storage(),
                                         account_id_, OrchardPool::kOrchard));
  sync_state_->OverrideShardTreeForTesting(
      OrchardPool::kIronwood, account_id_,
      orchard::CreateShardTreeForTesting(sync_state_->orchard_storage(),
                                         account_id_, OrchardPool::kIronwood));
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
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  // Testing tree has prune depth of 10.
  EXPECT_EQ(
      10u,
      storage().CheckpointCount(OrchardPool::kOrchard, account_id()).value());
  EXPECT_EQ(40u, storage()
                     .MinCheckpointId(OrchardPool::kOrchard, account_id())
                     .value()
                     .value());
  EXPECT_EQ(76u, storage()
                     .MaxCheckpointId(OrchardPool::kOrchard, account_id())
                     .value()
                     .value());
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
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 50;

  auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kOrchard, account_id(), {input}, 1);
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

  auto result = CreateResultForTesting(OrchardTreeState(),
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 3;
  auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kOrchard, account_id(), {input}, 1);
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
  auto result = CreateResultForTesting(OrchardTreeState(),
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  EXPECT_EQ(
      10u,
      storage().CheckpointCount(OrchardPool::kOrchard, account_id()).value());
  EXPECT_EQ(40u, storage()
                     .MinCheckpointId(OrchardPool::kOrchard, account_id())
                     .value()
                     .value());
  EXPECT_EQ(76u, storage()
                     .MaxCheckpointId(OrchardPool::kOrchard, account_id())
                     .value()
                     .value());
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
    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(commitments), 1000, "1000");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
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
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(commitments), 1000, "1000");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
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
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(commitments), 1000, "1000");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  EXPECT_EQ(
      3u,
      storage().CheckpointCount(OrchardPool::kOrchard, account_id()).value());
  EXPECT_EQ(1u, storage()
                    .MinCheckpointId(OrchardPool::kOrchard, account_id())
                    .value()
                    .value());
  EXPECT_EQ(3u, storage()
                    .MaxCheckpointId(OrchardPool::kOrchard, account_id())
                    .value()
                    .value());
}

TEST_F(OrchardSyncStateTest, GetSpendableNotes_NoRegisteredAccount) {
  OrchardAddrRawPart internal_addr;
  internal_addr.fill(3);
  auto get_spendable_notes_result = sync_state()->GetSpendableNotes(
      OrchardPool::kOrchard, account_id(), internal_addr);
  EXPECT_TRUE(get_spendable_notes_result.has_value());
  EXPECT_FALSE(get_spendable_notes_result.value().has_value());
}

TEST_F(OrchardSyncStateTest, GetSpendableNotes_FilterByAddress_And_Anchor) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardAddrRawPart internal_addr;
  internal_addr.fill(3);

  {
    std::vector<OrchardCommitment> commitments;
    std::vector<OrchardNote> notes;
    for (uint32_t i = 1000; i < 1050; i++) {
      OrchardNote note;
      note.amount = 10;
      note.block_id = i;
      note.addr.fill(2);
      note.nullifier.fill(i - 1000u);
      note.note_version = 2;
      notes.push_back(note);
      if (i == 1025) {
        commitments.push_back(
            CreateCommitment(CreateMockCommitmentValue(i, 2), true, i));
      } else {
        commitments.push_back(CreateCommitment(CreateMockCommitmentValue(i, 2),
                                               true, std::nullopt));
      }
    }

    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(commitments), 1049, "1049");
    result.orchard.discovered_notes = notes;

    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  auto get_spendable_notes_result = sync_state()->GetSpendableNotes(
      OrchardPool::kOrchard, account_id(), internal_addr);
  EXPECT_TRUE(get_spendable_notes_result.has_value());
  EXPECT_TRUE(get_spendable_notes_result.value().has_value());
  EXPECT_EQ(get_spendable_notes_result.value()->spendable_notes.size(), 26u);
  EXPECT_EQ(get_spendable_notes_result.value()->all_notes.size(), 50u);
}

TEST_F(OrchardSyncStateTest, GetSpendableNotes_FilterByAddress_External) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardAddrRawPart internal_addr;
  internal_addr.fill(3);

  {
    std::vector<OrchardCommitment> commitments;
    std::vector<OrchardNote> notes;
    for (uint32_t i = 1000; i < 1050; i++) {
      OrchardNote note;
      note.amount = 10;
      note.block_id = i;
      note.addr.fill(2);
      note.nullifier.fill(i - 1000u);
      note.note_version = 2;
      notes.push_back(note);
      commitments.push_back(
          CreateCommitment(CreateMockCommitmentValue(i, 2), true, i));
    }

    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(commitments), 1049, "1049");
    result.orchard.discovered_notes = notes;

    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  auto get_spendable_notes_result = sync_state()->GetSpendableNotes(
      OrchardPool::kOrchard, account_id(), internal_addr);
  EXPECT_TRUE(get_spendable_notes_result.has_value());
  EXPECT_TRUE(get_spendable_notes_result.value().has_value());
  EXPECT_EQ(get_spendable_notes_result.value()->spendable_notes.size(), 40u);
  EXPECT_EQ(get_spendable_notes_result.value()->all_notes.size(), 50u);
}

TEST_F(OrchardSyncStateTest, GetSpendableNotes_FilterByAddress_Internal) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardAddrRawPart internal_addr;
  internal_addr.fill(2);

  {
    std::vector<OrchardCommitment> commitments;
    std::vector<OrchardNote> notes;
    for (uint32_t i = 1000; i < 1050; i++) {
      OrchardNote note;
      note.amount = 10;
      note.block_id = i;
      note.addr.fill(2);
      note.nullifier.fill(i - 1000u);
      note.note_version = 2;
      notes.push_back(note);
      commitments.push_back(
          CreateCommitment(CreateMockCommitmentValue(i, 2), true, i));
    }

    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(commitments), 1049, "1049");
    result.orchard.discovered_notes = notes;

    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  auto get_spendable_notes_result = sync_state()->GetSpendableNotes(
      OrchardPool::kOrchard, account_id(), internal_addr);
  EXPECT_TRUE(get_spendable_notes_result.has_value());
  EXPECT_TRUE(get_spendable_notes_result.value().has_value());
  EXPECT_EQ(get_spendable_notes_result.value()->spendable_notes.size(), 46u);
  EXPECT_EQ(get_spendable_notes_result.value()->all_notes.size(), 50u);
}

TEST_F(OrchardSyncStateTest, GetSpendableNotes_NoAnchor) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardAddrRawPart internal_addr;
  internal_addr.fill(3);

  {
    auto result = CreateResultForTesting(
        OrchardTreeState(), std::vector<OrchardCommitment>(), 1000, "1000");
    for (uint32_t i = 1000; i < 1050; i++) {
      OrchardNote note;
      note.amount = 10;
      note.block_id = i;
      note.addr.fill(2);
      note.nullifier.fill(i - 1000u);
      note.note_version = 2;
      result.orchard.discovered_notes.push_back(note);
    }

    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  auto get_spendable_notes_result = sync_state()->GetSpendableNotes(
      OrchardPool::kOrchard, account_id(), internal_addr);
  EXPECT_TRUE(get_spendable_notes_result.has_value());
  EXPECT_TRUE(get_spendable_notes_result.value().has_value());
  // Since no checkpoints were added we drop all notes we have.
  EXPECT_EQ(get_spendable_notes_result.value()->spendable_notes.size(), 0u);
  EXPECT_EQ(get_spendable_notes_result.value()->all_notes.size(), 50u);
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

  auto result = CreateResultForTesting(OrchardTreeState(),
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  {
    OrchardInput input;
    input.note.orchard_commitment_tree_position = 2;
    auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
        OrchardPool::kOrchard, account_id(), {input}, 1);
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

  auto result = CreateResultForTesting(OrchardTreeState(),
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  {
    OrchardInput input;
    input.note.orchard_commitment_tree_position = 2;
    auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
        OrchardPool::kOrchard, account_id(), {input}, 2);
    EXPECT_FALSE(witness_result.has_value());
  }
}

TEST_F(OrchardSyncStateTest, Rewind_ToMarkedHeight) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  {
    std::vector<OrchardCommitment> commitments;

    for (int i = 0; i < 100; i++) {
      switch (i) {
        case 2:
          commitments.push_back(CreateCommitment(
              CreateMockCommitmentValue(i, kDefaultCommitmentSeed), true, 1));
          break;
        case 50:
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
    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(commitments), 1000, "1000");
    {
      OrchardNote note;
      note.block_id = 1;
      note.amount = 10000;
      note.nullifier.fill(1);
      note.note_version = 2;
      result.orchard.discovered_notes.push_back(note);
    }
    {
      OrchardNote note;
      note.block_id = 2;
      note.amount = 10000;
      note.nullifier.fill(2);
      note.note_version = 2;
      result.orchard.discovered_notes.push_back(note);
    }
    {
      OrchardNoteSpend spend;
      spend.block_id = 2;
      spend.nullifier.fill(1);
      result.orchard.found_spends.push_back(spend);
    }
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  EXPECT_EQ(1u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes.size());
  EXPECT_EQ(2u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes[0]
                    .block_id);

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 2;
  auto expected_witness = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kOrchard, account_id(), {input}, 2);

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            sync_state()->Rewind(account_id(), 1, "1").value());

  EXPECT_EQ(1u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes.size());
  EXPECT_EQ(1u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes[0]
                    .block_id);

  {
    OrchardTreeState tree_state;
    tree_state.block_height = 3;
    tree_state.tree_size = 3;

    std::vector<OrchardCommitment> commitments;
    for (int i = 3; i < 100; i++) {
      switch (i) {
        case 50:
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
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(commitments), 1000, "1000");
    {
      OrchardNote note;
      note.block_id = 2;
      note.amount = 10000;
      note.nullifier.fill(2);
      note.note_version = 2;
      result.orchard.discovered_notes.push_back(note);
    }
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  EXPECT_EQ(2u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes.size());
  EXPECT_EQ(1u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes[0]
                    .block_id);
  EXPECT_EQ(2u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes[1]
                    .block_id);

  auto actual_witness = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kOrchard, account_id(), {input}, 2);
  EXPECT_EQ(expected_witness.value()[0].witness.value(),
            actual_witness.value()[0].witness.value());
}

TEST_F(OrchardSyncStateTest, Rewind) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

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

    auto result = CreateResultForTesting(OrchardTreeState(),
                                         std::move(commitments), 1000, "1000");
    {
      OrchardNote note;
      note.block_id = 1;
      note.amount = 10000;
      note.nullifier.fill(1);
      note.note_version = 2;
      result.orchard.discovered_notes.push_back(note);
    }
    {
      OrchardNote note;
      note.block_id = 2;
      note.amount = 10000;
      note.nullifier.fill(2);
      note.note_version = 2;
      result.orchard.discovered_notes.push_back(note);
    }
    {
      OrchardNoteSpend spend;
      spend.block_id = 3;
      spend.nullifier.fill(1);
      result.orchard.found_spends.push_back(spend);
    }

    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  EXPECT_EQ(1u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes.size());
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            sync_state()->Rewind(account_id(), 2, "2").value());
  // Nullifier was deleted so we should have 2 spendable notes now.
  EXPECT_EQ(2u, sync_state()
                    ->GetSpendableNotes(OrchardPool::kOrchard, account_id(), {})
                    .value()
                    ->all_notes.size());

  {
    std::vector<OrchardCommitment> commitments;

    for (int j = 0; j < 5; j++) {
      if (j == 3) {
        commitments.push_back(
            CreateCommitment(CreateMockCommitmentValue(j, 5), false, 3));
      } else {
        commitments.push_back(CreateCommitment(CreateMockCommitmentValue(j, 5),
                                               false, std::nullopt));
      }
    }

    OrchardTreeState tree_state;
    tree_state.block_height = 2;
    // Truncate was on position 5, so 6 elements left in the tree
    tree_state.tree_size = 6;
    auto result = CreateResultForTesting(std::move(tree_state),
                                         std::move(commitments), 2000, "2000");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  {
    OrchardInput input;
    input.note.orchard_commitment_tree_position = 2;
    auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
        OrchardPool::kOrchard, account_id(), {input}, 2);
    // Since checkpoint #2 was deleted we shouldn't be able to calc witness
    EXPECT_FALSE(witness_result.has_value());
  }

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 2;
  auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kOrchard, account_id(), {input}, 1);
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

  auto result = CreateResultForTesting(OrchardTreeState(),
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  EXPECT_FALSE(sync_state()->Rewind(account_id(), 2, "2").has_value());
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

  auto result = CreateResultForTesting(OrchardTreeState(),
                                       std::move(commitments), 1000, "1000");
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 2;
  auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kOrchard, account_id(), {input}, 1);
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

TEST_F(OrchardSyncStateTest,
       IronwoodPool_GetSpendableNotes_FilterByAddress_And_Anchor) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature, {{"zcash_ironwood_enabled", "true"}});

  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardAddrRawPart internal_addr;
  internal_addr.fill(3);

  {
    std::vector<OrchardCommitment> commitments;
    std::vector<OrchardNote> notes;
    for (uint32_t i = 1000; i < 1050; i++) {
      OrchardNote note;
      note.amount = 10;
      note.block_id = i;
      note.addr.fill(2);
      note.nullifier.fill(i - 1000u);
      note.note_version = 2;
      notes.push_back(note);
      if (i == 1025) {
        commitments.push_back(
            CreateCommitment(CreateMockCommitmentValue(i, 2), true, i));
      } else {
        commitments.push_back(CreateCommitment(CreateMockCommitmentValue(i, 2),
                                               true, std::nullopt));
      }
    }

    auto result = CreateResultForTesting(
        OrchardTreeState(), std::vector<OrchardCommitment>(), 1049, "1049");
    result.ironwood = CreateIronwoodPoolResultForTesting(
        OrchardTreeState(), std::move(commitments), 1049, "1049");
    result.ironwood->discovered_notes = notes;

    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  // Orchard pool is untouched by an ironwood-only scan result.
  auto orchard_result = sync_state()->GetSpendableNotes(
      OrchardPool::kOrchard, account_id(), internal_addr);
  EXPECT_TRUE(orchard_result.has_value());
  EXPECT_TRUE(orchard_result.value().has_value());
  EXPECT_EQ(orchard_result.value()->all_notes.size(), 0u);

  auto get_spendable_notes_result = sync_state()->GetSpendableNotes(
      OrchardPool::kIronwood, account_id(), internal_addr);
  EXPECT_TRUE(get_spendable_notes_result.has_value());
  EXPECT_TRUE(get_spendable_notes_result.value().has_value());
  EXPECT_EQ(get_spendable_notes_result.value()->spendable_notes.size(), 26u);
  EXPECT_EQ(get_spendable_notes_result.value()->all_notes.size(), 50u);
}

TEST_F(OrchardSyncStateTest, OrchardAndIronwoodPoolsAreIndependent) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature, {{"zcash_ironwood_enabled", "true"}});

  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardAddrRawPart internal_addr;
  internal_addr.fill(3);

  std::vector<OrchardCommitment> orchard_commitments;
  std::vector<OrchardNote> orchard_notes;
  for (uint32_t i = 1000; i < 1010; i++) {
    OrchardNote note;
    note.amount = 10;
    note.block_id = i;
    note.addr.fill(2);
    note.nullifier.fill(i - 1000u);
    note.note_version = 2;
    orchard_notes.push_back(note);
    orchard_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(i, 2), true, i));
  }

  std::vector<OrchardCommitment> ironwood_commitments;
  std::vector<OrchardNote> ironwood_notes;
  for (uint32_t i = 1000; i < 1020; i++) {
    OrchardNote note;
    note.amount = 20;
    note.block_id = i;
    note.addr.fill(2);
    note.nullifier.fill(i - 1000u + 50);
    note.note_version = 2;
    ironwood_notes.push_back(note);
    ironwood_commitments.push_back(
        CreateCommitment(CreateMockCommitmentValue(i, 3), true, i));
  }

  auto result = CreateResultForTesting(
      OrchardTreeState(), std::move(orchard_commitments), 1049, "1049");
  result.orchard.discovered_notes = orchard_notes;
  result.ironwood = CreateIronwoodPoolResultForTesting(
      OrchardTreeState(), std::move(ironwood_commitments), 1049, "1049");
  result.ironwood->discovered_notes = ironwood_notes;

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  auto orchard_result = sync_state()->GetSpendableNotes(
      OrchardPool::kOrchard, account_id(), internal_addr);
  ASSERT_TRUE(orchard_result.has_value());
  ASSERT_TRUE(orchard_result.value().has_value());
  EXPECT_EQ(orchard_result.value()->all_notes.size(), 10u);
  for (const auto& note : orchard_result.value()->all_notes) {
    EXPECT_EQ(note.amount, 10u);
  }

  auto ironwood_result = sync_state()->GetSpendableNotes(
      OrchardPool::kIronwood, account_id(), internal_addr);
  ASSERT_TRUE(ironwood_result.has_value());
  ASSERT_TRUE(ironwood_result.value().has_value());
  EXPECT_EQ(ironwood_result.value()->all_notes.size(), 20u);
  for (const auto& note : ironwood_result.value()->all_notes) {
    EXPECT_EQ(note.amount, 20u);
  }
}

TEST_F(OrchardSyncStateTest, IronwoodPool_SimpleInsertWitness) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature, {{"zcash_ironwood_enabled", "true"}});

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

  auto result = CreateResultForTesting(
      OrchardTreeState(), std::vector<OrchardCommitment>(), 1000, "1000");
  result.ironwood = CreateIronwoodPoolResultForTesting(
      OrchardTreeState(), std::move(commitments), 1000, "1000");

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 2;
  // The orchard pool is untouched, so it has no checkpoint #1 to witness
  // against.
  EXPECT_FALSE(sync_state()
                   ->CalculateWitnessForCheckpoint(OrchardPool::kOrchard,
                                                   account_id(), {input}, 1)
                   .has_value());

  auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kIronwood, account_id(), {input}, 1);
  EXPECT_TRUE(witness_result.has_value());
  // Same commitments/positions/tree height as the orchard SimpleInsert case,
  // applied to the independent ironwood shard tree: identical witness.
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

TEST_F(OrchardSyncStateTest, Rewind_IronwoodPool_Enabled) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  {
    std::vector<OrchardCommitment> orchard_commitments;
    std::vector<OrchardCommitment> ironwood_commitments;
    for (int i = 0; i < 5; i++) {
      std::optional<uint32_t> checkpoint =
          i == 2 ? std::optional<uint32_t>(1) : std::nullopt;
      orchard_commitments.push_back(
          CreateCommitment(CreateMockCommitmentValue(i, kDefaultCommitmentSeed),
                           false, checkpoint));
      ironwood_commitments.push_back(CreateCommitment(
          CreateMockCommitmentValue(i, kDefaultCommitmentSeed + 10), false,
          checkpoint));
    }
    auto result = CreateResultForTesting(
        OrchardTreeState(), std::move(orchard_commitments), 2, "2");
    result.ironwood = CreateIronwoodPoolResultForTesting(
        OrchardTreeState(), std::move(ironwood_commitments), 2, "2");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  {
    OrchardTreeState orchard_tree_state;
    orchard_tree_state.block_height = 2;
    orchard_tree_state.tree_size = 5;
    OrchardTreeState ironwood_tree_state;
    ironwood_tree_state.block_height = 2;
    ironwood_tree_state.tree_size = 5;

    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(CreateCommitment(
        CreateMockCommitmentValue(5, kDefaultCommitmentSeed), false, 2u));
    std::vector<OrchardCommitment> ironwood_commitments;
    ironwood_commitments.push_back(CreateCommitment(
        CreateMockCommitmentValue(5, kDefaultCommitmentSeed + 10), false, 2u));

    auto result = CreateResultForTesting(
        std::move(orchard_tree_state), std::move(orchard_commitments), 4, "4");
    result.ironwood = CreateIronwoodPoolResultForTesting(
        std::move(ironwood_tree_state), std::move(ironwood_commitments), 4,
        "4");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  ASSERT_EQ(
      2u,
      storage().CheckpointCount(OrchardPool::kOrchard, account_id()).value());
  ASSERT_EQ(
      2u,
      storage().CheckpointCount(OrchardPool::kIronwood, account_id()).value());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature, {{"zcash_ironwood_enabled", "true"}});

  // TruncateToCheckpoint(2) removes checkpoint #2 (and later), keeping only
  // checkpoint #1.
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            sync_state()->Rewind(account_id(), 2, "2").value());

  EXPECT_EQ(
      1u,
      storage().CheckpointCount(OrchardPool::kOrchard, account_id()).value());
  // With the feature enabled, Rewind() also truncates the ironwood tree.
  EXPECT_EQ(
      1u,
      storage().CheckpointCount(OrchardPool::kIronwood, account_id()).value());
}

TEST_F(OrchardSyncStateTest, Rewind_IronwoodPool_Disabled) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  {
    std::vector<OrchardCommitment> orchard_commitments;
    std::vector<OrchardCommitment> ironwood_commitments;
    for (int i = 0; i < 5; i++) {
      std::optional<uint32_t> checkpoint =
          i == 2 ? std::optional<uint32_t>(1) : std::nullopt;
      orchard_commitments.push_back(
          CreateCommitment(CreateMockCommitmentValue(i, kDefaultCommitmentSeed),
                           false, checkpoint));
      ironwood_commitments.push_back(CreateCommitment(
          CreateMockCommitmentValue(i, kDefaultCommitmentSeed + 10), false,
          checkpoint));
    }
    auto result = CreateResultForTesting(
        OrchardTreeState(), std::move(orchard_commitments), 2, "2");
    result.ironwood = CreateIronwoodPoolResultForTesting(
        OrchardTreeState(), std::move(ironwood_commitments), 2, "2");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  {
    OrchardTreeState orchard_tree_state;
    orchard_tree_state.block_height = 2;
    orchard_tree_state.tree_size = 5;
    OrchardTreeState ironwood_tree_state;
    ironwood_tree_state.block_height = 2;
    ironwood_tree_state.tree_size = 5;

    std::vector<OrchardCommitment> orchard_commitments;
    orchard_commitments.push_back(CreateCommitment(
        CreateMockCommitmentValue(5, kDefaultCommitmentSeed), false, 2u));
    std::vector<OrchardCommitment> ironwood_commitments;
    ironwood_commitments.push_back(CreateCommitment(
        CreateMockCommitmentValue(5, kDefaultCommitmentSeed + 10), false, 2u));

    auto result = CreateResultForTesting(
        std::move(orchard_tree_state), std::move(orchard_commitments), 4, "4");
    result.ironwood = CreateIronwoodPoolResultForTesting(
        std::move(ironwood_tree_state), std::move(ironwood_commitments), 4,
        "4");
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              sync_state()
                  ->ApplyScanResults(account_id(), std::move(result))
                  .value());
  }

  ASSERT_EQ(
      2u,
      storage().CheckpointCount(OrchardPool::kOrchard, account_id()).value());
  ASSERT_EQ(
      2u,
      storage().CheckpointCount(OrchardPool::kIronwood, account_id()).value());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_ironwood_enabled", "false"}});

  // TruncateToCheckpoint(2) removes checkpoint #2 (and later), keeping only
  // checkpoint #1.
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            sync_state()->Rewind(account_id(), 2, "2").value());

  EXPECT_EQ(
      1u,
      storage().CheckpointCount(OrchardPool::kOrchard, account_id()).value());
  // With the feature disabled, Rewind() skips truncating the ironwood shard
  // tree entirely, even though the reorg still happened.
  EXPECT_EQ(
      2u,
      storage().CheckpointCount(OrchardPool::kIronwood, account_id()).value());
}

// The following tests verify that each OrchardSyncState accessor taking a
// `pool` argument short-circuits for the Ironwood pool when the feature is
// disabled, even when real data for that pool already exists in storage.

TEST_F(OrchardSyncStateTest, GetSpendableNotes_IronwoodPool_FeatureDisabled) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardNote note;
  note.amount = 20;
  note.block_id = 1000;
  note.addr.fill(2);
  note.nullifier.fill(1);
  note.note_version = 2;

  auto result = CreateResultForTesting(
      OrchardTreeState(), std::vector<OrchardCommitment>(), 1000, "1000");
  result.ironwood = CreateIronwoodPoolResultForTesting(
      OrchardTreeState(), std::vector<OrchardCommitment>(), 1000, "1000");
  result.ironwood->discovered_notes = {note};

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  // Confirm the note was actually persisted for the ironwood pool.
  ASSERT_EQ(1u, storage()
                    .GetSpendableNotes(OrchardPool::kIronwood, account_id())
                    .value()
                    .size());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_ironwood_enabled", "false"}});

  OrchardAddrRawPart internal_addr;
  internal_addr.fill(3);
  auto ironwood_result = sync_state()->GetSpendableNotes(
      OrchardPool::kIronwood, account_id(), internal_addr);
  EXPECT_TRUE(ironwood_result.has_value());
  EXPECT_FALSE(ironwood_result.value().has_value());
}

TEST_F(OrchardSyncStateTest, GetNullifiers_IronwoodPool_FeatureDisabled) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardNoteSpend spend;
  spend.block_id = 1000;
  spend.nullifier.fill(7);
  std::vector<OrchardNoteSpend> spends = {spend};
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            storage()
                .UpdateNotes(OrchardPool::kIronwood, account_id(),
                             std::vector<OrchardNote>(), spends, 1000, "1000")
                .value());

  // Confirm the nullifier was actually persisted for the ironwood pool.
  ASSERT_EQ(1u, storage()
                    .GetNullifiers(OrchardPool::kIronwood, account_id())
                    .value()
                    .size());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_ironwood_enabled", "false"}});

  auto nullifiers_result =
      sync_state()->GetNullifiers(OrchardPool::kIronwood, account_id());
  EXPECT_TRUE(nullifiers_result.has_value());
  EXPECT_TRUE(nullifiers_result.value().empty());
}

TEST_F(OrchardSyncStateTest,
       CalculateWitnessForCheckpoint_IronwoodPool_FeatureDisabled) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

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

  auto result = CreateResultForTesting(
      OrchardTreeState(), std::vector<OrchardCommitment>(), 1000, "1000");
  result.ironwood = CreateIronwoodPoolResultForTesting(
      OrchardTreeState(), std::move(commitments), 1000, "1000");

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      sync_state()->ApplyScanResults(account_id(), std::move(result)).value());

  // Confirm checkpoint #1 was actually persisted for the ironwood pool.
  ASSERT_EQ(
      1u,
      storage().CheckpointCount(OrchardPool::kIronwood, account_id()).value());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_ironwood_enabled", "false"}});

  OrchardInput input;
  input.note.orchard_commitment_tree_position = 2;
  auto witness_result = sync_state()->CalculateWitnessForCheckpoint(
      OrchardPool::kIronwood, account_id(), {input}, 1);
  EXPECT_TRUE(witness_result.has_value());
  EXPECT_TRUE(witness_result.value().empty());
}

TEST_F(OrchardSyncStateTest, GetLatestShardIndex_IronwoodPool_FeatureDisabled) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  OrchardShard ironwood_shard;
  ironwood_shard.address.index = 3;
  ironwood_shard.address.level = 1;
  ironwood_shard.shard_data = std::vector<uint8_t>({1, 1, 1, 1});
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            storage()
                .PutShard(OrchardPool::kIronwood, account_id(), ironwood_shard)
                .value());

  // Confirm the shard was actually persisted for the ironwood pool.
  ASSERT_EQ(3u, storage()
                    .GetLatestShardIndex(OrchardPool::kIronwood, account_id())
                    .value()
                    .value());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_ironwood_enabled", "false"}});

  auto shard_index_result =
      sync_state()->GetLatestShardIndex(OrchardPool::kIronwood, account_id());
  EXPECT_TRUE(shard_index_result.has_value());
  EXPECT_FALSE(shard_index_result.value().has_value());
}

TEST_F(OrchardSyncStateTest, GetMinCheckpointId_IronwoodPool_FeatureDisabled) {
  EXPECT_TRUE(sync_state()->RegisterAccount(account_id(), 0u).has_value());

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            storage()
                .AddCheckpoint(OrchardPool::kIronwood, account_id(), 5,
                               OrchardCheckpoint(std::nullopt, {}))
                .value());

  // Confirm the checkpoint was actually persisted for the ironwood pool.
  ASSERT_EQ(5u, storage()
                    .MinCheckpointId(OrchardPool::kIronwood, account_id())
                    .value()
                    .value());

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_ironwood_enabled", "false"}});

  auto min_checkpoint_result =
      sync_state()->GetMinCheckpointId(OrchardPool::kIronwood, account_id());
  EXPECT_TRUE(min_checkpoint_result.has_value());
  EXPECT_FALSE(min_checkpoint_result.value().has_value());
}

}  // namespace brave_wallet
