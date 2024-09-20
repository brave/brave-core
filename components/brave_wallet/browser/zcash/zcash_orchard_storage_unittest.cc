/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_storage.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

OrchardShardAddress AddressFromOrchardShard(const OrchardShard& item) {
  return item.address;
}

}  // namespace

class OrchardStorageTest : public testing::Test {
 public:
  OrchardStorageTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override;

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<ZCashOrchardStorage> orchard_storage_;
};

void OrchardStorageTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
  orchard_storage_ = base::WrapRefCounted(new ZCashOrchardStorage(db_path));
}

TEST_F(OrchardStorageTest, AccountMeta) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  {
    auto result = orchard_storage_->GetAccountMeta(account_id_1.Clone());
    EXPECT_FALSE(result.has_value());
  }

  EXPECT_TRUE(
      orchard_storage_->RegisterAccount(account_id_1.Clone(), 100, "hash")
          .has_value());

  {
    auto result = orchard_storage_->GetAccountMeta(account_id_1.Clone());
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->account_birthday, 100u);
    EXPECT_EQ(result->latest_scanned_block_id, 100u);
    EXPECT_EQ(result->latest_scanned_block_hash, "hash");
  }

  {
    // Failed to insert same account
    EXPECT_EQ(
        orchard_storage_->RegisterAccount(account_id_1.Clone(), 200, "hash")
            .error()
            .error_code,
        ZCashOrchardStorage::ErrorCode::kFailedToExecuteStatement);
  }

  // Insert second account
  EXPECT_TRUE(
      orchard_storage_->RegisterAccount(account_id_2.Clone(), 200, "hash")
          .has_value());
  {
    auto result = orchard_storage_->GetAccountMeta(account_id_2.Clone());
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->account_birthday, 200u);
    EXPECT_EQ(result->latest_scanned_block_id, 200u);
    EXPECT_EQ(result->latest_scanned_block_hash, "hash");
  }
}

TEST_F(OrchardStorageTest, PutDiscoveredNotes) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  EXPECT_TRUE(
      orchard_storage_->RegisterAccount(account_id_1.Clone(), 100, "hash")
          .has_value());
  EXPECT_TRUE(
      orchard_storage_->RegisterAccount(account_id_2.Clone(), 100, "hash")
          .has_value());

  // Update notes for account 1
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id_1, 101, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 105, 2));

    orchard_storage_->UpdateNotes(account_id_1.Clone(), notes, {}, 200,
                                  "hash200");
  }

  // Update notes for account 2
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id_2, 111, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 115, 2));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 117, 3));

    orchard_storage_->UpdateNotes(account_id_2.Clone(), notes, {}, 200,
                                  "hash200");
  }

  // Check account_1 spendable notes
  {
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1.Clone());
    EXPECT_EQ(2u, account_1_spendable_notes->size());
    SortByBlockId(*account_1_spendable_notes);
    EXPECT_EQ(account_1_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_1, 101, 1));
    EXPECT_EQ(account_1_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_1, 105, 2));
  }

  // Check account_2 spendable notes
  {
    auto account_2_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_2.Clone());
    EXPECT_EQ(3u, account_2_spendable_notes->size());
    SortByBlockId(*account_2_spendable_notes);
    EXPECT_EQ(account_2_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_2, 111, 1));
    EXPECT_EQ(account_2_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_2, 115, 2));
    EXPECT_EQ(account_2_spendable_notes.value()[2],
              GenerateMockOrchardNote(account_id_2, 117, 3));
  }

  // Update notes for account 1
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNullifier> nullifiers;

    // Add 1 note, spend 1 note
    notes.push_back(GenerateMockOrchardNote(account_id_1, 201, 3));
    nullifiers.push_back(GenerateMockNullifier(account_id_1, 203, 1));

    orchard_storage_->UpdateNotes(account_id_1.Clone(), notes, nullifiers, 300,
                                  "hash300");
  }

  // Update notes for account 2
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNullifier> nullifiers;

    // Add 1 note, spend 2 notes
    notes.push_back(GenerateMockOrchardNote(account_id_2, 211, 4));
    nullifiers.push_back(GenerateMockNullifier(account_id_2, 222, 2));
    nullifiers.push_back(GenerateMockNullifier(account_id_2, 233, 3));

    orchard_storage_->UpdateNotes(account_id_2.Clone(), notes, nullifiers, 300,
                                  "hash300");
  }

  // Check account_1 spendable notes
  {
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1.Clone());
    EXPECT_EQ(2u, account_1_spendable_notes->size());
    SortByBlockId(*account_1_spendable_notes);
    EXPECT_EQ(account_1_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_1, 105, 2));
    EXPECT_EQ(account_1_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_1, 201, 3));
  }

  // Check account_2 spendable notes
  {
    auto account_2_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_2.Clone());
    EXPECT_EQ(2u, account_2_spendable_notes->size());
    SortByBlockId(*account_2_spendable_notes);
    EXPECT_EQ(account_2_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_2, 111, 1));
    EXPECT_EQ(account_2_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_2, 211, 4));
  }

  // Accounts meta updated
  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_1.Clone());
    EXPECT_EQ(account_meta->latest_scanned_block_id, 300u);
    EXPECT_EQ(account_meta->latest_scanned_block_hash, "hash300");
  }

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_2.Clone());
    EXPECT_EQ(account_meta->latest_scanned_block_id, 300u);
    EXPECT_EQ(account_meta->latest_scanned_block_hash, "hash300");
  }
}

TEST_F(OrchardStorageTest, HandleChainReorg) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  EXPECT_TRUE(
      orchard_storage_->RegisterAccount(account_id_1.Clone(), 100, "hash")
          .has_value());
  EXPECT_TRUE(
      orchard_storage_->RegisterAccount(account_id_2.Clone(), 100, "hash")
          .has_value());

  // Update notes for account 1
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNullifier> nullifiers;

    // Add 4 notes, spend 2 notes
    notes.push_back(GenerateMockOrchardNote(account_id_1, 101, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 102, 2));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 103, 3));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 104, 4));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 304, 5));

    nullifiers.push_back(GenerateMockNullifier(account_id_1, 102, 2));
    nullifiers.push_back(GenerateMockNullifier(account_id_1, 103, 3));

    orchard_storage_->UpdateNotes(account_id_1.Clone(), notes, nullifiers, 450,
                                  "hash450");
  }

  // Update notes for account 2
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNullifier> nullifiers;

    // Add 4 notes, spend 2 notes
    notes.push_back(GenerateMockOrchardNote(account_id_2, 211, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 212, 2));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 213, 3));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 414, 4));

    nullifiers.push_back(GenerateMockNullifier(account_id_2, 322, 2));
    nullifiers.push_back(GenerateMockNullifier(account_id_2, 333, 3));

    orchard_storage_->UpdateNotes(account_id_2.Clone(), notes, nullifiers, 500,
                                  "hash500");
  }

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_2.Clone());
    EXPECT_EQ(account_meta->latest_scanned_block_id, 500u);
    EXPECT_EQ(account_meta->latest_scanned_block_hash, "hash500");
    auto account_2_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_2.Clone());
    EXPECT_EQ(2u, account_2_spendable_notes->size());
    auto account_2_nullifiers =
        orchard_storage_->GetNullifiers(account_id_2.Clone());
    EXPECT_EQ(2u, account_2_nullifiers->size());
  }

  // Call handle chain reorg so notes and spends with index > 300 will be
  // deleted
  orchard_storage_->HandleChainReorg(account_id_2.Clone(), 300u, "hash300");

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_2.Clone());
    EXPECT_EQ(account_meta->latest_scanned_block_id, 300u);
    EXPECT_EQ(account_meta->latest_scanned_block_hash, "hash300");
  }

  // Unused account is untouched
  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_1.Clone());
    EXPECT_EQ(account_meta->latest_scanned_block_id, 450u);
    EXPECT_EQ(account_meta->latest_scanned_block_hash, "hash450");
  }

  // Check account_1 spendable notes
  // Notes for account 1 should be untouched since reorg wasn't launched for
  // account_1
  {
    auto account_1_nullifiers =
        orchard_storage_->GetNullifiers(account_id_1.Clone());
    EXPECT_EQ(2u, account_1_nullifiers->size());
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1.Clone());
    // Since we removed 2 nullifiers and 1 note there should be 3 spendable
    // notes
    EXPECT_EQ(3u, account_1_spendable_notes->size());
    SortByBlockId(*account_1_spendable_notes);
    EXPECT_EQ(account_1_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_1, 101, 1));
    EXPECT_EQ(account_1_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_1, 104, 4));
    EXPECT_EQ(account_1_spendable_notes.value()[2],
              GenerateMockOrchardNote(account_id_1, 304, 5));
  }

  // Check account_2 spendable notes
  {
    auto account_2_nullifiers =
        orchard_storage_->GetNullifiers(account_id_2.Clone());
    EXPECT_EQ(0u, account_2_nullifiers->size());
    auto account_2_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_2.Clone());
    // Since we removed 2 nullifiers and 1 note there should be 3 spendable
    // notes
    EXPECT_EQ(3u, account_2_spendable_notes->size());
    SortByBlockId(*account_2_spendable_notes);
    EXPECT_EQ(account_2_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_2, 211, 1));
    EXPECT_EQ(account_2_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_2, 212, 2));
    EXPECT_EQ(account_2_spendable_notes.value()[2],
              GenerateMockOrchardNote(account_id_2, 213, 3));
  }

  orchard_storage_->HandleChainReorg(account_id_1.Clone(), 0u, "hash0");
  // Account 1 was reorged on 0
  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_1.Clone());
    EXPECT_EQ(account_meta->latest_scanned_block_id, 0u);
    EXPECT_EQ(account_meta->latest_scanned_block_hash, "hash0");
  }

  // Check account_1 spendable notes
  // Account 1 was reorged on 0 so no nullifiers and notes should exist.
  {
    auto account_1_nullifiers =
        orchard_storage_->GetNullifiers(account_id_1.Clone());
    EXPECT_EQ(0u, account_1_nullifiers->size());
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1.Clone());
    EXPECT_EQ(0u, account_1_spendable_notes->size());
  }
}

TEST_F(OrchardStorageTest, Shards) {}

namespace {

OrchardShard CreateSubtreeRoot(size_t index, size_t level) {
  OrchardShard orchard_shard;
  orchard_shard.root_hash.fill(static_cast<uint8_t>(index));
  orchard_shard.address.index = index;
  orchard_shard.address.level = level;
  orchard_shard.contains_marked = false;
  return orchard_shard;
}

OrchardShard CreateShard(size_t index, size_t level) {
  OrchardShard orchard_shard;
  orchard_shard.root_hash.fill(static_cast<uint8_t>(index));
  orchard_shard.address.index = index;
  orchard_shard.address.level = level;
  orchard_shard.contains_marked = false;
  orchard_shard.shard_data = std::vector<uint8_t>({0, 0, 0, 0});
  return orchard_shard;
}

}  // namespace

TEST_F(OrchardStorageTest, InsertSubtreeRoots) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  std::vector<OrchardShard> level_1_roots;
  {
    for (int i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(i, 9));
    }
    EXPECT_TRUE(
        orchard_storage_->PutShardRoots(account_id.Clone(), level_1_roots)
            .value());
  }

  {
    std::vector<OrchardShardAddress> level_1_addrs;
    base::ranges::transform(level_1_roots, std::back_inserter(level_1_addrs),
                            AddressFromOrchardShard);

    auto result = orchard_storage_->GetShardRoots(account_id.Clone(), 9);
    EXPECT_EQ(result.value(), level_1_addrs);
  }
}

TEST_F(OrchardStorageTest, TruncateSubtreeRoots) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  std::vector<OrchardShard> level_1_roots;
  {
    for (int i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(i, 1));
    }
    EXPECT_TRUE(
        orchard_storage_->PutShardRoots(account_id.Clone(), level_1_roots)
            .value());
  }

  EXPECT_TRUE(orchard_storage_->TruncateShards(account_id.Clone(), 5).value());
  {
    std::vector<OrchardShardAddress> level_1_addrs;
    base::ranges::transform(level_1_roots, std::back_inserter(level_1_addrs),
                            AddressFromOrchardShard);

    auto result = orchard_storage_->GetShardRoots(account_id.Clone(), 1);
    EXPECT_EQ(result.value(),
              std::vector<OrchardShardAddress>(level_1_addrs.begin(),
                                               level_1_addrs.begin() + 5));
  }
}

TEST_F(OrchardStorageTest, TruncateShards) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  {
    for (uint32_t i = 0; i < 10; i++) {
      EXPECT_TRUE(
          orchard_storage_->PutShard(account_id.Clone(), CreateShard(i, 1))
              .value());
    }
  }

  EXPECT_TRUE(orchard_storage_->TruncateShards(account_id.Clone(), 5).value());
  for (uint32_t i = 0; i < 5; i++) {
    EXPECT_EQ(CreateShard(i, 1),
              **(orchard_storage_->GetShard(account_id.Clone(),
                                            OrchardShardAddress(1, i))));
  }

  EXPECT_EQ(std::nullopt, *(orchard_storage_->GetShard(
                              account_id.Clone(), OrchardShardAddress(1, 6))));
}

TEST_F(OrchardStorageTest, ShardOverridesSubtreeRoot) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  std::vector<OrchardShard> level_1_roots;
  {
    for (int i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(i, 1));
    }
    EXPECT_TRUE(
        orchard_storage_->PutShardRoots(account_id.Clone(), level_1_roots)
            .value());
  }

  // Update existing shard
  OrchardShard new_shard;
  new_shard.address.index = 5;
  new_shard.address.level = 1;
  new_shard.root_hash.fill(5);
  new_shard.shard_data = std::vector<uint8_t>({5, 5, 5, 5});
  EXPECT_TRUE(
      orchard_storage_->PutShard(account_id.Clone(), new_shard).value());

  auto result =
      orchard_storage_->GetShard(account_id.Clone(), OrchardShardAddress{1, 5});
  EXPECT_EQ(*result.value(), new_shard);
}

TEST_F(OrchardStorageTest, InsertShards) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetLatestShardIndex(account_id.Clone()).value());
  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetShard(account_id.Clone(), OrchardShardAddress{1, 0})
          .value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->LastShard(account_id.Clone(), 1).value());

  std::vector<OrchardShard> level_1_roots;
  {
    for (uint8_t i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(i, 1));
    }
    EXPECT_TRUE(
        orchard_storage_->PutShardRoots(account_id.Clone(), level_1_roots)
            .value());
  }

  OrchardShard new_shard;
  new_shard.address.index = 11;
  new_shard.address.level = 1;
  new_shard.root_hash.fill(11);
  new_shard.shard_data = std::vector<uint8_t>({1, 1, 1, 1});

  EXPECT_TRUE(
      orchard_storage_->PutShard(account_id.Clone(), new_shard).value());

  {
    auto result = orchard_storage_->GetShard(account_id.Clone(),
                                             OrchardShardAddress{1, 11});
    EXPECT_EQ(*result.value(), new_shard);
  }

  {
    for (uint8_t i = 0; i < 10; i++) {
      auto result = orchard_storage_->GetShard(account_id.Clone(),
                                               OrchardShardAddress{1, i});
      EXPECT_EQ(*result.value(), level_1_roots[i]);
    }
  }

  EXPECT_EQ(11u, orchard_storage_->GetLatestShardIndex(account_id.Clone())
                     .value()
                     .value());
  EXPECT_EQ(new_shard,
            orchard_storage_->LastShard(account_id.Clone(), 1).value());
}

TEST_F(OrchardStorageTest, RemoveChekpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint1)
          .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = std::nullopt;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 2, checkpoint2)
          .value());

  EXPECT_TRUE(
      orchard_storage_->RemoveCheckpoint(account_id.Clone(), 1).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpoint(account_id.Clone(), 1).value());
  EXPECT_EQ(
      OrchardCheckpointBundle(2, checkpoint2),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 2).value().value());
}

TEST_F(OrchardStorageTest, CheckpointId) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  EXPECT_EQ(std::nullopt,
            orchard_storage_->MinCheckpointId(account_id.Clone()).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->MaxCheckpointId(account_id.Clone()).value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint1)
          .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint2.tree_state_position = 2;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 2, checkpoint2)
          .value());

  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({5});
  checkpoint3.tree_state_position = 3;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 3, checkpoint3)
          .value());

  OrchardCheckpoint checkpoint4;
  checkpoint4.marks_removed = std::vector<uint32_t>();
  checkpoint4.tree_state_position = std::nullopt;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 4, checkpoint4)
          .value());

  EXPECT_EQ(1, orchard_storage_->MinCheckpointId(account_id.Clone()).value());
  EXPECT_EQ(4, orchard_storage_->MaxCheckpointId(account_id.Clone()).value());
}

TEST_F(OrchardStorageTest, CheckpointAtPosition) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint1)
          .value());
  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 2, checkpoint2)
          .value());
  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({7, 8, 9});
  checkpoint3.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 3, checkpoint3)
          .value());

  EXPECT_EQ(1u, orchard_storage_->GetCheckpointAtDepth(account_id.Clone(), 2)
                    .value()
                    .value());
  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetCheckpointAtDepth(account_id.Clone(), 5).value());
}

TEST_F(OrchardStorageTest, TruncateCheckpoints_OutOfBoundry) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint1)
          .value());

  EXPECT_TRUE(
      orchard_storage_->TruncateCheckpoints(account_id.Clone(), 3).value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 1).value().value());
}

TEST_F(OrchardStorageTest, TruncateCheckpoints) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint1)
          .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint2.tree_state_position = 2;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 2, checkpoint2)
          .value());

  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({5});
  checkpoint3.tree_state_position = 3;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 3, checkpoint3)
          .value());

  OrchardCheckpoint checkpoint4;
  checkpoint4.marks_removed = std::vector<uint32_t>();
  checkpoint4.tree_state_position = std::nullopt;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 4, checkpoint4)
          .value());

  EXPECT_TRUE(
      orchard_storage_->TruncateCheckpoints(account_id.Clone(), 3).value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 1).value().value());
  EXPECT_EQ(
      OrchardCheckpointBundle(2, checkpoint2),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 2).value().value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpoint(account_id.Clone(), 3).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpoint(account_id.Clone(), 4).value());
}

TEST_F(OrchardStorageTest, AddCheckpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint1)
          .value());
  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = std::nullopt;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 2, checkpoint2)
          .value());
  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>();
  checkpoint3.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 3, checkpoint3)
          .value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 1).value().value());
  EXPECT_EQ(
      OrchardCheckpointBundle(2, checkpoint2),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 2).value().value());
  EXPECT_EQ(
      OrchardCheckpointBundle(3, checkpoint3),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 3).value().value());
}

TEST_F(OrchardStorageTest, AddSameCheckpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());
  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1, 2, 3});
    checkpoint.tree_state_position = 4;
    EXPECT_TRUE(
        orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint)
            .value());
    EXPECT_TRUE(
        orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint)
            .value());

    EXPECT_EQ(
        OrchardCheckpointBundle(1, checkpoint),
        orchard_storage_->GetCheckpoint(account_id.Clone(), 1).value().value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1, 2, 3});
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_TRUE(
        orchard_storage_->AddCheckpoint(account_id.Clone(), 2, checkpoint)
            .value());
    EXPECT_TRUE(
        orchard_storage_->AddCheckpoint(account_id.Clone(), 2, checkpoint)
            .value());

    EXPECT_EQ(
        OrchardCheckpointBundle(2, checkpoint),
        orchard_storage_->GetCheckpoint(account_id.Clone(), 2).value().value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>();
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_TRUE(
        orchard_storage_->AddCheckpoint(account_id.Clone(), 3, checkpoint)
            .value());
    EXPECT_TRUE(
        orchard_storage_->AddCheckpoint(account_id.Clone(), 3, checkpoint)
            .value());

    EXPECT_EQ(
        OrchardCheckpointBundle(3, checkpoint),
        orchard_storage_->GetCheckpoint(account_id.Clone(), 3).value().value());
  }
}

TEST_F(OrchardStorageTest, AddChekpoint_ErrorOnConflict) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id.Clone(), 100, "hash")
                  .has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_TRUE(
      orchard_storage_->AddCheckpoint(account_id.Clone(), 1, checkpoint1)
          .value());

  OrchardCheckpoint checkpoint_different_marks_removed = checkpoint1;
  checkpoint_different_marks_removed.marks_removed =
      std::vector<uint32_t>({1, 2});
  EXPECT_FALSE(orchard_storage_
                   ->AddCheckpoint(account_id.Clone(), 1,
                                   checkpoint_different_marks_removed)
                   .has_value());

  OrchardCheckpoint checkpoint_different_position1 = checkpoint1;
  checkpoint_different_position1.tree_state_position = 7;
  EXPECT_FALSE(
      orchard_storage_
          ->AddCheckpoint(account_id.Clone(), 1, checkpoint_different_position1)
          .has_value());

  OrchardCheckpoint checkpoint_different_position2 = checkpoint1;
  checkpoint_different_position2.tree_state_position = std::nullopt;
  EXPECT_FALSE(
      orchard_storage_
          ->AddCheckpoint(account_id.Clone(), 1, checkpoint_different_position2)
          .has_value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1),
      orchard_storage_->GetCheckpoint(account_id.Clone(), 1).value().value());
}

}  // namespace brave_wallet
