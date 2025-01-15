/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class OrchardStorageTest : public testing::Test {
 public:
  OrchardStorageTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override;

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<OrchardStorage> orchard_storage_;
};

void OrchardStorageTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
  orchard_storage_ = std::make_unique<OrchardStorage>(db_path);
  orchard_storage_->EnsureDbInit();
}

TEST_F(OrchardStorageTest, AccountMeta) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  {
    auto result = orchard_storage_->GetAccountMeta(account_id_1);
    EXPECT_FALSE(result.value());
  }

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RegisterAccount(account_id_1, 100).value());

  {
    auto result = orchard_storage_->GetAccountMeta(account_id_1);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->account_birthday, 100u);
    EXPECT_FALSE(result.value()->latest_scanned_block_id);
    EXPECT_FALSE(result.value()->latest_scanned_block_hash);
  }

  {
    // Failed to insert same account
    EXPECT_EQ(
        orchard_storage_->RegisterAccount(account_id_1, 200).error().error_code,
        OrchardStorage::ErrorCode::kFailedToExecuteStatement);
  }

  // Insert second account
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id_2, 200).has_value());
  {
    auto result = orchard_storage_->GetAccountMeta(account_id_2);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->account_birthday, 200u);
    EXPECT_FALSE(result.value()->latest_scanned_block_id);
    EXPECT_FALSE(result.value()->latest_scanned_block_hash);
  }
}

TEST_F(OrchardStorageTest, PutDiscoveredNotes) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id_1, 100).has_value());
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id_2, 100).has_value());

  // Update notes for account 1
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id_1, 101, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 105, 2));

    EXPECT_TRUE(
        orchard_storage_->UpdateNotes(account_id_1, notes, {}, 200, "hash200")
            .has_value());
  }

  // Update notes for account 2
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id_2, 111, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 115, 2));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 117, 3));

    EXPECT_TRUE(
        orchard_storage_->UpdateNotes(account_id_2, notes, {}, 200, "hash200")
            .has_value());
  }

  // Check account_1 spendable notes
  {
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1);
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
        orchard_storage_->GetSpendableNotes(account_id_2);
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
    std::vector<OrchardNoteSpend> spends;

    // Add 1 note, spend 1 note
    notes.push_back(GenerateMockOrchardNote(account_id_1, 201, 3));
    spends.push_back(
        OrchardNoteSpend{203, GenerateMockNullifier(account_id_1, 1)});

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(account_id_1, notes, spends, 300, "hash300")
                    .has_value());
  }

  // Update notes for account 2
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNoteSpend> spends;

    // Add 1 note, spend 2 notes
    notes.push_back(GenerateMockOrchardNote(account_id_2, 211, 4));
    spends.push_back(
        OrchardNoteSpend{222, GenerateMockNullifier(account_id_2, 2)});
    spends.push_back(
        OrchardNoteSpend{233, GenerateMockNullifier(account_id_2, 3)});

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(account_id_2, notes, spends, 300, "hash300")
                    .has_value());
  }

  // Check account_1 spendable notes
  {
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1);
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
        orchard_storage_->GetSpendableNotes(account_id_2);
    EXPECT_EQ(2u, account_2_spendable_notes->size());
    SortByBlockId(*account_2_spendable_notes);
    EXPECT_EQ(account_2_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_2, 111, 1));
    EXPECT_EQ(account_2_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_2, 211, 4));
  }

  // Accounts meta updated
  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_1);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_id, 300u);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_hash, "hash300");
  }

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_2);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_id, 300u);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_hash, "hash300");
  }
}

TEST_F(OrchardStorageTest, HandleChainReorg) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id_1, 100).has_value());
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id_2, 100).has_value());

  // Update notes for account 1
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNoteSpend> spends;

    // Add 4 notes, spend 2 notes
    notes.push_back(GenerateMockOrchardNote(account_id_1, 101, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 102, 2));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 103, 3));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 104, 4));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 304, 5));

    spends.push_back(
        OrchardNoteSpend{102, GenerateMockNullifier(account_id_1, 2)});
    spends.push_back(
        OrchardNoteSpend{103, GenerateMockNullifier(account_id_1, 3)});

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(account_id_1, notes, spends, 450, "hash450")
                    .has_value());
  }

  // Update notes for account 2
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNoteSpend> spends;

    // Add 4 notes, spend 2 notes
    notes.push_back(GenerateMockOrchardNote(account_id_2, 211, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 212, 2));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 213, 3));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 414, 4));

    spends.push_back(
        OrchardNoteSpend{322, GenerateMockNullifier(account_id_2, 2)});
    spends.push_back(
        OrchardNoteSpend{333, GenerateMockNullifier(account_id_2, 3)});

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(account_id_2, notes, spends, 500, "hash500")
                    .has_value());
  }

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_2);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_id, 500u);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_hash, "hash500");
    auto account_2_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_2);
    EXPECT_EQ(2u, account_2_spendable_notes->size());
    auto account_2_nullifiers = orchard_storage_->GetNullifiers(account_id_2);
    EXPECT_EQ(2u, account_2_nullifiers->size());
  }

  // Call handle chain reorg so notes and spends with index > 300 will be
  // deleted
  EXPECT_TRUE(orchard_storage_->HandleChainReorg(account_id_2, 300u, "hash300")
                  .has_value());

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_2);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_id, 300u);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_hash, "hash300");
  }

  // Unused account is untouched
  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_1);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_id, 450u);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_hash, "hash450");
  }

  // Check account_1 spendable notes
  // Notes for account 1 should be untouched since reorg wasn't launched for
  // account_1
  {
    auto account_1_nullifiers = orchard_storage_->GetNullifiers(account_id_1);
    EXPECT_EQ(2u, account_1_nullifiers->size());
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1);
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
    auto account_2_nullifiers = orchard_storage_->GetNullifiers(account_id_2);
    EXPECT_EQ(0u, account_2_nullifiers->size());
    auto account_2_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_2);
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

  EXPECT_TRUE(orchard_storage_->HandleChainReorg(account_id_1, 0u, "hash0")
                  .has_value());
  // Account 1 was reorged on 0
  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_1);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_id.value(), 0u);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_hash.value(), "hash0");
  }

  // Check account_1 spendable notes
  // Account 1 was reorged on 0 so no nullifiers and notes should exist.
  {
    auto account_1_nullifiers = orchard_storage_->GetNullifiers(account_id_1);
    EXPECT_EQ(0u, account_1_nullifiers->size());
    auto account_1_spendable_notes =
        orchard_storage_->GetSpendableNotes(account_id_1);
    EXPECT_EQ(0u, account_1_spendable_notes->size());
  }
}

TEST_F(OrchardStorageTest, Shards) {}

namespace {

zcash::mojom::SubtreeRootPtr CreateSubtreeRoot(size_t level, size_t index) {
  zcash::mojom::SubtreeRootPtr root = zcash::mojom::SubtreeRoot::New();
  root->root_hash = std::vector<uint8_t>(kOrchardShardTreeHashSize, index);
  root->complete_block_hash =
      std::vector<uint8_t>(kOrchardCompleteBlockHashSize, index);
  root->complete_block_height = 0;
  return root;
}

OrchardShard CreateShard(size_t index, size_t level) {
  OrchardShard orchard_shard;
  orchard_shard.root_hash = OrchardShardRootHash();
  orchard_shard.root_hash->fill(static_cast<uint8_t>(index));
  orchard_shard.address.index = index;
  orchard_shard.address.level = level;
  orchard_shard.shard_data = std::vector<uint8_t>({0, 0, 0, 0});
  return orchard_shard;
}

}  // namespace

TEST_F(OrchardStorageTest, InsertSubtreeRoots_BlockHashConflict) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  std::vector<zcash::mojom::SubtreeRootPtr> level_1_roots;
  level_1_roots.push_back(CreateSubtreeRoot(9, 0));
  level_1_roots.push_back(CreateSubtreeRoot(9, 0));
  EXPECT_FALSE(orchard_storage_
                   ->UpdateSubtreeRoots(account_id, 0, std::move(level_1_roots))
                   .has_value());
}

TEST_F(OrchardStorageTest, InsertSubtreeRoots) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  {
    std::vector<zcash::mojom::SubtreeRootPtr> level_1_roots;
    for (uint32_t i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(9, i));
    }
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->UpdateSubtreeRoots(account_id, 0, std::move(level_1_roots))
                  .value());
  }

  {
    std::vector<OrchardShardAddress> level_1_addrs;
    for (uint32_t i = 0; i < 10; i++) {
      level_1_addrs.push_back(OrchardShardAddress{9, i});
    }
    auto result = orchard_storage_->GetShardRoots(account_id, 9);

    EXPECT_EQ(result.value(), level_1_addrs);
  }
}

TEST_F(OrchardStorageTest, TruncateSubtreeRoots) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  {
    std::vector<zcash::mojom::SubtreeRootPtr> level_1_roots;
    for (int i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(1, i));
    }
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->UpdateSubtreeRoots(account_id, 0, std::move(level_1_roots))
                  .value());
  }

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->TruncateShards(account_id, 5).value());
  {
    std::vector<OrchardShardAddress> addresses_after_truncate;
    for (uint32_t i = 0; i < 5; i++) {
      addresses_after_truncate.push_back(OrchardShardAddress{1, i});
    }
    auto result = orchard_storage_->GetShardRoots(account_id, 1);
    EXPECT_EQ(result.value(), addresses_after_truncate);
  }
}

TEST_F(OrchardStorageTest, TruncateShards) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  {
    for (uint32_t i = 0; i < 10; i++) {
      EXPECT_EQ(
          OrchardStorage::Result::kSuccess,
          orchard_storage_->PutShard(account_id, CreateShard(i, 1)).value());
    }
  }

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->TruncateShards(account_id, 5).value());

  for (uint32_t i = 0; i < 5; i++) {
    EXPECT_EQ(CreateShard(i, 1), **(orchard_storage_->GetShard(
                                     account_id, OrchardShardAddress(1, i))));
  }

  EXPECT_EQ(std::nullopt, *(orchard_storage_->GetShard(
                              account_id, OrchardShardAddress(1, 6))));
}

TEST_F(OrchardStorageTest, ShardOverridesSubtreeRoot) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  {
    std::vector<zcash::mojom::SubtreeRootPtr> level_1_roots;
    for (uint32_t i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(1, i));
    }
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->UpdateSubtreeRoots(account_id, 0, std::move(level_1_roots))
                  .value());
  }

  // Update existing shard
  OrchardShard new_shard;
  new_shard.root_hash = OrchardShardRootHash();
  new_shard.address.index = 5;
  new_shard.address.level = 1;
  new_shard.root_hash->fill(5);
  new_shard.shard_data = std::vector<uint8_t>({5, 5, 5, 5});
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->PutShard(account_id, new_shard).value());

  auto result =
      orchard_storage_->GetShard(account_id, OrchardShardAddress{1, 5});
  EXPECT_EQ(*result.value(), new_shard);
}

TEST_F(OrchardStorageTest, InsertShards) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetLatestShardIndex(account_id).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetShard(account_id, OrchardShardAddress{1, 0})
                .value());
  EXPECT_EQ(std::nullopt, orchard_storage_->LastShard(account_id, 1).value());

  {
    std::vector<zcash::mojom::SubtreeRootPtr> level_1_roots;
    for (uint32_t i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(1, i));
    }
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->UpdateSubtreeRoots(account_id, 0, std::move(level_1_roots))
                  .value());
  }

  OrchardShard new_shard;
  new_shard.root_hash = OrchardShardRootHash();
  new_shard.address.index = 11;
  new_shard.address.level = 1;
  new_shard.root_hash->fill(11);
  new_shard.shard_data = std::vector<uint8_t>({1, 1, 1, 1});

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->PutShard(account_id, new_shard).value());

  {
    auto result =
        orchard_storage_->GetShard(account_id, OrchardShardAddress{1, 11});
    EXPECT_EQ(*result.value(), new_shard);
  }

  {
    for (uint32_t i = 0; i < 10; i++) {
      auto result =
          orchard_storage_->GetShard(account_id, OrchardShardAddress{1, i});
      auto root = CreateSubtreeRoot(1, i);
      EXPECT_EQ(std::vector<uint8_t>(std::begin(*result.value()->root_hash),
                                     std::end(*result.value()->root_hash)),
                root->root_hash);
    }
  }

  EXPECT_EQ(11u,
            orchard_storage_->GetLatestShardIndex(account_id).value().value());
  EXPECT_EQ(new_shard, orchard_storage_->LastShard(account_id, 1).value());
}

TEST_F(OrchardStorageTest, RemoveChekpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 1, checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 2, checkpoint2.Clone())
                .value());

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RemoveCheckpoint(account_id, 1).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpoint(account_id, 1).value());
  EXPECT_EQ(OrchardCheckpointBundle(2, checkpoint2.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 2).value().value());
  // Unexisting checkpoint.
  EXPECT_EQ(OrchardStorage::Result::kNone,
            orchard_storage_->RemoveCheckpoint(account_id, 3).value());
}

TEST_F(OrchardStorageTest, CheckpointId) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  EXPECT_EQ(std::nullopt,
            orchard_storage_->MinCheckpointId(account_id).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->MaxCheckpointId(account_id).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetMaxCheckpointedHeight(account_id, 100000, 0)
                .value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 1, checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint2.tree_state_position = 2;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 2, checkpoint2.Clone())
                .value());

  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({5});
  checkpoint3.tree_state_position = 3;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 3, checkpoint3.Clone())
                .value());

  OrchardCheckpoint checkpoint4;
  checkpoint4.marks_removed = std::vector<uint32_t>();
  checkpoint4.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 4, checkpoint4.Clone())
                .value());

  EXPECT_EQ(1, orchard_storage_->MinCheckpointId(account_id).value());
  EXPECT_EQ(4, orchard_storage_->MaxCheckpointId(account_id).value());
  EXPECT_EQ(4, orchard_storage_->GetMaxCheckpointedHeight(account_id, 100000, 0)
                   .value());
  EXPECT_EQ(
      2, orchard_storage_->GetMaxCheckpointedHeight(account_id, 3, 0).value());
}

TEST_F(OrchardStorageTest, CheckpointAtPosition) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpointAtDepth(account_id, 2).value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 1, checkpoint1.Clone())
                .value());
  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 2, checkpoint2.Clone())
                .value());
  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({7, 8, 9});
  checkpoint3.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 3, checkpoint3.Clone())
                .value());

  EXPECT_EQ(
      1u,
      orchard_storage_->GetCheckpointAtDepth(account_id, 2).value().value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpointAtDepth(account_id, 5).value());
}

TEST_F(OrchardStorageTest, TruncateCheckpoints_OutOfBoundary) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 1, checkpoint1.Clone())
                .value());

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->TruncateCheckpoints(account_id, 3).value());

  EXPECT_EQ(OrchardCheckpointBundle(1, checkpoint1.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 1).value().value());
}

TEST_F(OrchardStorageTest, TruncateCheckpoints) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 1, checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint2.tree_state_position = 2;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 2, checkpoint2.Clone())
                .value());

  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({5});
  checkpoint3.tree_state_position = 3;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 3, checkpoint3.Clone())
                .value());

  OrchardCheckpoint checkpoint4;
  checkpoint4.marks_removed = std::vector<uint32_t>();
  checkpoint4.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 4, checkpoint4.Clone())
                .value());

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->TruncateCheckpoints(account_id, 3).value());

  EXPECT_EQ(OrchardCheckpointBundle(1, checkpoint1.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 1).value().value());
  EXPECT_EQ(OrchardCheckpointBundle(2, checkpoint2.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 2).value().value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpoint(account_id, 3).value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpoint(account_id, 4).value());
}

TEST_F(OrchardStorageTest, AddCheckpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->GetCheckpoint(account_id, 1).value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 1, checkpoint1.Clone())
                .value());
  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 2, checkpoint2.Clone())
                .value());
  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>();
  checkpoint3.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 3, checkpoint3.Clone())
                .value());

  EXPECT_EQ(OrchardCheckpointBundle(1, checkpoint1.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 1).value().value());
  EXPECT_EQ(OrchardCheckpointBundle(2, checkpoint2.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 2).value().value());
  EXPECT_EQ(OrchardCheckpointBundle(3, checkpoint3.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 3).value().value());
}

TEST_F(OrchardStorageTest, AddSameCheckpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());
  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1, 2, 3});
    checkpoint.tree_state_position = 4;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 1, checkpoint.Clone())
                  .value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 1, checkpoint.Clone())
                  .value());

    EXPECT_EQ(OrchardCheckpointBundle(1, checkpoint.Clone()),
              orchard_storage_->GetCheckpoint(account_id, 1).value().value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1, 2, 3});
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 2, checkpoint.Clone())
                  .value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 2, checkpoint.Clone())
                  .value());

    EXPECT_EQ(OrchardCheckpointBundle(2, checkpoint.Clone()),
              orchard_storage_->GetCheckpoint(account_id, 2).value().value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>();
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 3, checkpoint.Clone())
                  .value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 3, checkpoint.Clone())
                  .value());

    EXPECT_EQ(OrchardCheckpointBundle(3, checkpoint.Clone()),
              orchard_storage_->GetCheckpoint(account_id, 3).value().value());
  }
}

TEST_F(OrchardStorageTest, AddChekpoint_ErrorOnConflict) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->AddCheckpoint(account_id, 1, checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint_different_marks_removed;
  checkpoint_different_marks_removed.tree_state_position =
      checkpoint1.tree_state_position;
  checkpoint_different_marks_removed.marks_removed =
      std::vector<uint32_t>({1, 2});
  EXPECT_FALSE(orchard_storage_
                   ->AddCheckpoint(account_id, 1,
                                   checkpoint_different_marks_removed.Clone())
                   .has_value());

  OrchardCheckpoint checkpoint_different_position1;
  checkpoint_different_position1.marks_removed = checkpoint1.marks_removed;
  checkpoint_different_position1.tree_state_position = 7;
  EXPECT_FALSE(
      orchard_storage_
          ->AddCheckpoint(account_id, 1, checkpoint_different_position1.Clone())
          .has_value());

  OrchardCheckpoint checkpoint_different_position2;
  checkpoint_different_position2.tree_state_position = std::nullopt;
  checkpoint_different_position2.marks_removed = checkpoint1.marks_removed;
  EXPECT_FALSE(
      orchard_storage_
          ->AddCheckpoint(account_id, 1, checkpoint_different_position2.Clone())
          .has_value());

  EXPECT_EQ(OrchardCheckpointBundle(1, checkpoint1.Clone()),
            orchard_storage_->GetCheckpoint(account_id, 1).value().value());
}

TEST_F(OrchardStorageTest, ResetAccountSyncState) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  // Update notes for account 1
  {
    std::vector<OrchardNote> notes;
    std::vector<OrchardNoteSpend> spends;

    notes.push_back(GenerateMockOrchardNote(account_id, 101, 1));
    notes.push_back(GenerateMockOrchardNote(account_id, 105, 2));

    spends.push_back(
        OrchardNoteSpend{322, GenerateMockNullifier(account_id, 2)});
    spends.push_back(
        OrchardNoteSpend{333, GenerateMockNullifier(account_id, 3)});

    EXPECT_TRUE(
        orchard_storage_->UpdateNotes(account_id, notes, spends, 200, "hash200")
            .has_value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>();
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 3, checkpoint.Clone())
                  .value());
  }

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->ResetAccountSyncState(account_id).value());

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id);
    EXPECT_FALSE(account_meta.value()->latest_scanned_block_id);
    EXPECT_FALSE(account_meta.value()->latest_scanned_block_hash);
  }

  EXPECT_EQ(0u,
            orchard_storage_->GetCheckpoints(account_id, 100).value().size());
  EXPECT_EQ(0u, orchard_storage_->GetSpendableNotes(account_id).value().size());
  EXPECT_EQ(0u, orchard_storage_->GetNullifiers(account_id).value().size());
}

TEST_F(OrchardStorageTest, UpdateCheckpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  // Create a single checkpoint.
  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>();
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddCheckpoint(account_id, 3, checkpoint.Clone())
                  .value());
  }

  // Update existing checkpoint.
  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1});
    checkpoint.tree_state_position = 15;
    EXPECT_EQ(
        OrchardStorage::Result::kSuccess,
        orchard_storage_->UpdateCheckpoint(account_id, 3, checkpoint).value());
    auto get_checkpoint_result = orchard_storage_->GetCheckpoint(account_id, 3);
    EXPECT_TRUE(get_checkpoint_result.has_value());
    OrchardCheckpointBundle checkpoint_bundle =
        std::move(**get_checkpoint_result);
    EXPECT_EQ(3u, checkpoint_bundle.checkpoint_id);
    EXPECT_EQ(15, checkpoint_bundle.checkpoint.tree_state_position);
    EXPECT_EQ(std::vector<uint32_t>({1}),
              checkpoint_bundle.checkpoint.marks_removed);
  }

  // Update non-existing checkpoint.
  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1});
    checkpoint.tree_state_position = 15;
    EXPECT_EQ(
        OrchardStorage::Result::kNone,
        orchard_storage_->UpdateCheckpoint(account_id, 5, checkpoint).value());
  }
}

TEST_F(OrchardStorageTest, Transactionally) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);

  EXPECT_FALSE(orchard_storage_->GetAccountMeta(account_id_1).value());

  {
    auto tx = orchard_storage_->Transactionally();
    EXPECT_TRUE(tx.has_value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->RegisterAccount(account_id_1, 100).value());
  }

  EXPECT_FALSE(orchard_storage_->GetAccountMeta(account_id_1).value());

  {
    auto tx = orchard_storage_->Transactionally();
    EXPECT_TRUE(tx.has_value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->RegisterAccount(account_id_1, 100).value());
    EXPECT_TRUE(tx->Commit().has_value());
  }

  EXPECT_TRUE(orchard_storage_->GetAccountMeta(account_id_1).has_value());
}

}  // namespace brave_wallet
