/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "sql/database.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Creates a v2 orchard SQLite database at |db_path| with minimal test data for
// |account_id|. Used to verify the v2→v3 schema upgrade.
void PopulateOrchardV2DatabaseForTesting(  // IN-TEST
    const base::FilePath& db_path,
    const mojom::AccountIdPtr& account_id) {
  sql::Database database(sql::Database::Tag("PopulateOrchardV2Database"));
  CHECK(database.Open(db_path));

  sql::MetaTable meta_table;
  CHECK(meta_table.Init(&database, /*version=*/2, /*compatible_version=*/2));

  CHECK(
      database.Execute("CREATE TABLE notes ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "account_id TEXT NOT NULL,"
                       "amount INTEGER NOT NULL,"
                       "addr BLOB NOT NULL,"
                       "block_id INTEGER NOT NULL,"
                       "commitment_tree_position INTEGER,"
                       "nullifier BLOB NOT NULL UNIQUE,"
                       "rho BLOB NOT NULL,"
                       "rseed BLOB NOT NULL);"));

  CHECK(
      database.Execute("CREATE TABLE spent_notes ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "account_id TEXT NOT NULL,"
                       "spent_block_id INTEGER NOT NULL,"
                       "nullifier BLOB NOT NULL UNIQUE);"));

  CHECK(
      database.Execute("CREATE TABLE account_meta ("
                       "account_id TEXT NOT NULL PRIMARY KEY,"
                       "account_birthday INTEGER NOT NULL,"
                       "latest_scanned_block INTEGER,"
                       "latest_scanned_block_hash TEXT);"));

  CHECK(database.Execute(
      "CREATE TABLE shard_tree ("
      "account_id TEXT NOT NULL,"
      "shard_index INTEGER NOT NULL,"
      "subtree_end_height INTEGER,"
      "root_hash BLOB,"
      "shard_data BLOB,"
      "CONSTRAINT shard_index_unique UNIQUE (shard_index, account_id),"
      "CONSTRAINT root_unique UNIQUE (root_hash, account_id));"));

  CHECK(
      database.Execute("CREATE TABLE checkpoints ("
                       "account_id TEXT NOT NULL,"
                       "checkpoint_id INTEGER NOT NULL,"
                       "position INTEGER,"
                       "PRIMARY KEY (checkpoint_id));"));

  CHECK(
      database.Execute("CREATE TABLE checkpoints_mark_removed ("
                       "account_id TEXT NOT NULL,"
                       "checkpoint_id INTEGER NOT NULL,"
                       "mark_removed_position INTEGER NOT NULL,"
                       "FOREIGN KEY (checkpoint_id) REFERENCES "
                       "checkpoints(checkpoint_id) ON DELETE CASCADE,"
                       "CONSTRAINT spend_position_unique UNIQUE "
                       "(checkpoint_id, mark_removed_position, account_id));"));

  CHECK(
      database.Execute("CREATE TABLE shard_tree_cap ("
                       "account_id TEXT NOT NULL,"
                       "cap_data BLOB NOT NULL);"));

  {
    sql::Statement stmt(database.GetUniqueStatement(
        "INSERT INTO account_meta (account_id, account_birthday, "
        "latest_scanned_block, latest_scanned_block_hash) "
        "VALUES (?, ?, ?, ?)"));
    stmt.BindString(0, account_id->unique_key);
    stmt.BindInt64(1, 100);
    stmt.BindInt64(2, 105);
    stmt.BindString(3, "hash105");
    CHECK(stmt.Run());
  }

  {
    std::vector<uint8_t> addr(43, 0);
    std::vector<uint8_t> nullifier(32, 1);
    std::vector<uint8_t> rho(32, 0);
    std::vector<uint8_t> rseed(32, 0);
    sql::Statement stmt(database.GetUniqueStatement(
        "INSERT INTO notes (account_id, amount, addr, block_id, "
        "commitment_tree_position, nullifier, rho, rseed) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    stmt.BindString(0, account_id->unique_key);
    stmt.BindInt64(1, 1000);
    stmt.BindBlob(2, addr);
    stmt.BindInt64(3, 101);
    stmt.BindInt64(4, 0);
    stmt.BindBlob(5, nullifier);
    stmt.BindBlob(6, rho);
    stmt.BindBlob(7, rseed);
    CHECK(stmt.Run());
  }

  {
    sql::Statement stmt(database.GetUniqueStatement(
        "INSERT INTO checkpoints (account_id, checkpoint_id, position) "
        "VALUES (?, ?, ?)"));
    stmt.BindString(0, account_id->unique_key);
    stmt.BindInt64(1, 1);
    stmt.BindInt64(2, 4);
    CHECK(stmt.Run());
  }

  // Marks removed for checkpoint 1, used to verify that
  // checkpoints_mark_removed data survives the checkpoints table rebuild
  // during the v2->v3 migration.
  for (uint32_t mark : {1, 2, 3}) {
    sql::Statement stmt(database.GetUniqueStatement(
        "INSERT INTO checkpoints_mark_removed (account_id, checkpoint_id, "
        "mark_removed_position) VALUES (?, ?, ?)"));
    stmt.BindString(0, account_id->unique_key);
    stmt.BindInt64(1, 1);
    stmt.BindInt64(2, mark);
    CHECK(stmt.Run());
  }
}

}  // namespace

class OrchardStorageTest : public testing::Test {
 public:
  OrchardStorageTest() = default;
  void SetUp() override;

  base::ScopedTempDir temp_dir_;
  std::unique_ptr<OrchardStorage> orchard_storage_;
};

void OrchardStorageTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  orchard_storage_ = std::make_unique<OrchardStorage>(
      temp_dir_.GetPath().AppendASCII("orchard.db"));
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

TEST_F(OrchardStorageTest, PutDiscoveredNotes_u64Value) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);

  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id_1, 100).has_value());

  // Update notes for account 1 with wrong values
  {
    std::vector<OrchardNote> notes;
    notes.push_back(
        GenerateMockOrchardNote(account_id_1, 101, 1, 0xFFFFFFFFFFFFFFFF));
    notes.push_back(
        GenerateMockOrchardNote(account_id_1, 105, 2, 0xAAFFFFFFFFFFFFFF));

    EXPECT_FALSE(orchard_storage_
                     ->UpdateNotes(OrchardPool::kOrchard, account_id_1, notes,
                                   {}, 200, "hash200")
                     .has_value());
  }

  // Update notes for account 1 with valid values
  {
    std::vector<OrchardNote> notes;
    notes.push_back(
        GenerateMockOrchardNote(account_id_1, 101, 1, 0xFFFFFFFFFFFFFFFF / 2));
    notes.push_back(GenerateMockOrchardNote(account_id_1, 105, 2,
                                            0xFFFFFFFFFFFFFFFF / 2 - 1));

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(OrchardPool::kOrchard, account_id_1, notes,
                                  {}, 200, "hash200")
                    .has_value());
  }

  // Check account_1 spendable notes
  {
    auto account_1_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_1);
    EXPECT_EQ(2u, account_1_spendable_notes->size());
    SortByBlockId(*account_1_spendable_notes);
    EXPECT_EQ(
        account_1_spendable_notes.value()[0],
        GenerateMockOrchardNote(account_id_1, 101, 1, 0xFFFFFFFFFFFFFFFF / 2));
    EXPECT_EQ(account_1_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_1, 105, 2,
                                      0xFFFFFFFFFFFFFFFF / 2 - 1));
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

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(OrchardPool::kOrchard, account_id_1, notes,
                                  {}, 200, "hash200")
                    .has_value());
  }

  // Update notes for account 2
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id_2, 111, 1));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 115, 2));
    notes.push_back(GenerateMockOrchardNote(account_id_2, 117, 3));

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(OrchardPool::kOrchard, account_id_2, notes,
                                  {}, 200, "hash200")
                    .has_value());
  }

  // Check account_1 spendable notes
  {
    auto account_1_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_1);
    EXPECT_EQ(2u, account_1_spendable_notes->size());
    SortByBlockId(*account_1_spendable_notes);
    EXPECT_EQ(account_1_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_1, 101, 1));
    EXPECT_EQ(account_1_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_1, 105, 2));
  }

  // Check account_2 spendable notes
  {
    auto account_2_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_2);
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
                    ->UpdateNotes(OrchardPool::kOrchard, account_id_1, notes,
                                  spends, 300, "hash300")
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
                    ->UpdateNotes(OrchardPool::kOrchard, account_id_2, notes,
                                  spends, 300, "hash300")
                    .has_value());
  }

  // Check account_1 spendable notes
  {
    auto account_1_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_1);
    EXPECT_EQ(2u, account_1_spendable_notes->size());
    SortByBlockId(*account_1_spendable_notes);
    EXPECT_EQ(account_1_spendable_notes.value()[0],
              GenerateMockOrchardNote(account_id_1, 105, 2));
    EXPECT_EQ(account_1_spendable_notes.value()[1],
              GenerateMockOrchardNote(account_id_1, 201, 3));
  }

  // Check account_2 spendable notes
  {
    auto account_2_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_2);
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
                    ->UpdateNotes(OrchardPool::kOrchard, account_id_1, notes,
                                  spends, 450, "hash450")
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
                    ->UpdateNotes(OrchardPool::kOrchard, account_id_2, notes,
                                  spends, 500, "hash500")
                    .has_value());
  }

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id_2);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_id, 500u);
    EXPECT_EQ(account_meta.value()->latest_scanned_block_hash, "hash500");
    auto account_2_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_2);
    EXPECT_EQ(2u, account_2_spendable_notes->size());
    auto account_2_nullifiers =
        orchard_storage_->GetNullifiers(OrchardPool::kOrchard, account_id_2);
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
    auto account_1_nullifiers =
        orchard_storage_->GetNullifiers(OrchardPool::kOrchard, account_id_1);
    EXPECT_EQ(2u, account_1_nullifiers->size());
    auto account_1_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_1);
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
        orchard_storage_->GetNullifiers(OrchardPool::kOrchard, account_id_2);
    EXPECT_EQ(0u, account_2_nullifiers->size());
    auto account_2_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_2);
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
    auto account_1_nullifiers =
        orchard_storage_->GetNullifiers(OrchardPool::kOrchard, account_id_1);
    EXPECT_EQ(0u, account_1_nullifiers->size());
    auto account_1_spendable_notes = orchard_storage_->GetSpendableNotes(
        OrchardPool::kOrchard, account_id_1);
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
                   ->UpdateSubtreeRoots(OrchardPool::kOrchard, account_id, 0,
                                        std::move(level_1_roots))
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
                  ->UpdateSubtreeRoots(OrchardPool::kOrchard, account_id, 0,
                                       std::move(level_1_roots))
                  .value());
  }

  {
    std::vector<OrchardShardAddress> level_1_addrs;
    for (uint32_t i = 0; i < 10; i++) {
      level_1_addrs.push_back(OrchardShardAddress{9, i});
    }
    auto result =
        orchard_storage_->GetShardRoots(OrchardPool::kOrchard, account_id, 9);

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
                  ->UpdateSubtreeRoots(OrchardPool::kOrchard, account_id, 0,
                                       std::move(level_1_roots))
                  .value());
  }

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->TruncateShards(OrchardPool::kOrchard, account_id, 5)
          .value());
  {
    std::vector<OrchardShardAddress> addresses_after_truncate;
    for (uint32_t i = 0; i < 5; i++) {
      addresses_after_truncate.push_back(OrchardShardAddress{1, i});
    }
    auto result =
        orchard_storage_->GetShardRoots(OrchardPool::kOrchard, account_id, 1);
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
          orchard_storage_
              ->PutShard(OrchardPool::kOrchard, account_id, CreateShard(i, 1))
              .value());
    }
  }

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->TruncateShards(OrchardPool::kOrchard, account_id, 5)
          .value());

  for (uint32_t i = 0; i < 5; i++) {
    EXPECT_EQ(CreateShard(i, 1),
              **(orchard_storage_->GetShard(OrchardPool::kOrchard, account_id,
                                            OrchardShardAddress(1, i))));
  }

  EXPECT_EQ(std::nullopt,
            *(orchard_storage_->GetShard(OrchardPool::kOrchard, account_id,
                                         OrchardShardAddress(1, 6))));
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
                  ->UpdateSubtreeRoots(OrchardPool::kOrchard, account_id, 0,
                                       std::move(level_1_roots))
                  .value());
  }

  // Update existing shard
  OrchardShard new_shard;
  new_shard.root_hash = OrchardShardRootHash();
  new_shard.address.index = 5;
  new_shard.address.level = 1;
  new_shard.root_hash->fill(5);
  new_shard.shard_data = std::vector<uint8_t>({5, 5, 5, 5});
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->PutShard(OrchardPool::kOrchard, account_id, new_shard)
          .value());

  auto result = orchard_storage_->GetShard(OrchardPool::kOrchard, account_id,
                                           OrchardShardAddress{1, 5});
  EXPECT_EQ(*result.value(), new_shard);
}

TEST_F(OrchardStorageTest, InsertShards) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetLatestShardIndex(OrchardPool::kOrchard, account_id)
          .value());
  EXPECT_EQ(std::nullopt, orchard_storage_
                              ->GetShard(OrchardPool::kOrchard, account_id,
                                         OrchardShardAddress{1, 0})
                              .value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->LastShard(OrchardPool::kOrchard, account_id, 1)
                .value());

  {
    std::vector<zcash::mojom::SubtreeRootPtr> level_1_roots;
    for (uint32_t i = 0; i < 10; i++) {
      level_1_roots.push_back(CreateSubtreeRoot(1, i));
    }
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->UpdateSubtreeRoots(OrchardPool::kOrchard, account_id, 0,
                                       std::move(level_1_roots))
                  .value());
  }

  OrchardShard new_shard;
  new_shard.root_hash = OrchardShardRootHash();
  new_shard.address.index = 11;
  new_shard.address.level = 1;
  new_shard.root_hash->fill(11);
  new_shard.shard_data = std::vector<uint8_t>({1, 1, 1, 1});

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->PutShard(OrchardPool::kOrchard, account_id, new_shard)
          .value());

  {
    auto result = orchard_storage_->GetShard(OrchardPool::kOrchard, account_id,
                                             OrchardShardAddress{1, 11});
    EXPECT_EQ(*result.value(), new_shard);
  }

  {
    for (uint32_t i = 0; i < 10; i++) {
      auto result = orchard_storage_->GetShard(
          OrchardPool::kOrchard, account_id, OrchardShardAddress{1, i});
      auto root = CreateSubtreeRoot(1, i);
      EXPECT_EQ(std::vector<uint8_t>(std::begin(*result.value()->root_hash),
                                     std::end(*result.value()->root_hash)),
                root->root_hash);
    }
  }

  EXPECT_EQ(11u, orchard_storage_
                     ->GetLatestShardIndex(OrchardPool::kOrchard, account_id)
                     .value()
                     .value());
  EXPECT_EQ(new_shard,
            orchard_storage_->LastShard(OrchardPool::kOrchard, account_id, 1)
                .value());
}

TEST_F(OrchardStorageTest, RemoveCheckpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 2,
                                checkpoint2.Clone())
                .value());

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->RemoveCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value());
  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value());
  EXPECT_EQ(
      OrchardCheckpointBundle(2, checkpoint2.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 2)
          .value()
          .value());
  // Unexisting checkpoint.
  EXPECT_EQ(
      OrchardStorage::Result::kNone,
      orchard_storage_->RemoveCheckpoint(OrchardPool::kOrchard, account_id, 3)
          .value());
}

TEST_F(OrchardStorageTest, CheckpointId) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  EXPECT_EQ(std::nullopt,
            orchard_storage_->MinCheckpointId(OrchardPool::kOrchard, account_id)
                .value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_->MaxCheckpointId(OrchardPool::kOrchard, account_id)
                .value());
  EXPECT_EQ(std::nullopt, orchard_storage_
                              ->GetMaxCheckpointedHeight(OrchardPool::kOrchard,
                                                         account_id, 100000, 0)
                              .value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint2.tree_state_position = 2;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 2,
                                checkpoint2.Clone())
                .value());

  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({5});
  checkpoint3.tree_state_position = 3;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                checkpoint3.Clone())
                .value());

  OrchardCheckpoint checkpoint4;
  checkpoint4.marks_removed = std::vector<uint32_t>();
  checkpoint4.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 4,
                                checkpoint4.Clone())
                .value());

  EXPECT_EQ(1,
            orchard_storage_->MinCheckpointId(OrchardPool::kOrchard, account_id)
                .value());
  EXPECT_EQ(4,
            orchard_storage_->MaxCheckpointId(OrchardPool::kOrchard, account_id)
                .value());
  EXPECT_EQ(4, orchard_storage_
                   ->GetMaxCheckpointedHeight(OrchardPool::kOrchard, account_id,
                                              100000, 0)
                   .value());
  EXPECT_EQ(
      2, orchard_storage_
             ->GetMaxCheckpointedHeight(OrchardPool::kOrchard, account_id, 3, 0)
             .value());
}

TEST_F(OrchardStorageTest, CheckpointAtPosition) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  EXPECT_EQ(std::nullopt,
            orchard_storage_
                ->GetCheckpointAtDepth(OrchardPool::kOrchard, account_id, 2)
                .value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                checkpoint1.Clone())
                .value());
  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 2,
                                checkpoint2.Clone())
                .value());
  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({7, 8, 9});
  checkpoint3.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                checkpoint3.Clone())
                .value());

  EXPECT_EQ(1u, orchard_storage_
                    ->GetCheckpointAtDepth(OrchardPool::kOrchard, account_id, 2)
                    .value()
                    .value());
  EXPECT_EQ(std::nullopt,
            orchard_storage_
                ->GetCheckpointAtDepth(OrchardPool::kOrchard, account_id, 5)
                .value());
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
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                checkpoint1.Clone())
                .value());

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->TruncateCheckpoints(OrchardPool::kOrchard, account_id, 3)
                .value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value()
          .value());
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
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint2.tree_state_position = 2;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 2,
                                checkpoint2.Clone())
                .value());

  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>({5});
  checkpoint3.tree_state_position = 3;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                checkpoint3.Clone())
                .value());

  OrchardCheckpoint checkpoint4;
  checkpoint4.marks_removed = std::vector<uint32_t>();
  checkpoint4.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 4,
                                checkpoint4.Clone())
                .value());

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->TruncateCheckpoints(OrchardPool::kOrchard, account_id, 3)
                .value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value()
          .value());
  EXPECT_EQ(
      OrchardCheckpointBundle(2, checkpoint2.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 2)
          .value()
          .value());
  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 3)
          .value());
  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 4)
          .value());
}

TEST_F(OrchardStorageTest, AddCheckpoint) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());
  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                checkpoint1.Clone())
                .value());
  OrchardCheckpoint checkpoint2;
  checkpoint2.marks_removed = std::vector<uint32_t>({4, 5, 6});
  checkpoint2.tree_state_position = std::nullopt;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 2,
                                checkpoint2.Clone())
                .value());
  OrchardCheckpoint checkpoint3;
  checkpoint3.marks_removed = std::vector<uint32_t>();
  checkpoint3.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                checkpoint3.Clone())
                .value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value()
          .value());
  EXPECT_EQ(
      OrchardCheckpointBundle(2, checkpoint2.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 2)
          .value()
          .value());
  EXPECT_EQ(
      OrchardCheckpointBundle(3, checkpoint3.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 3)
          .value()
          .value());
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
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                  checkpoint.Clone())
                  .value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                  checkpoint.Clone())
                  .value());

    EXPECT_EQ(
        OrchardCheckpointBundle(1, checkpoint.Clone()),
        orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
            .value()
            .value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1, 2, 3});
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 2,
                                  checkpoint.Clone())
                  .value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 2,
                                  checkpoint.Clone())
                  .value());

    EXPECT_EQ(
        OrchardCheckpointBundle(2, checkpoint.Clone()),
        orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 2)
            .value()
            .value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>();
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                  checkpoint.Clone())
                  .value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                  checkpoint.Clone())
                  .value());

    EXPECT_EQ(
        OrchardCheckpointBundle(3, checkpoint.Clone()),
        orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 3)
            .value()
            .value());
  }
}

TEST_F(OrchardStorageTest, AddCheckpoint_ErrorOnConflict) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id, 100).has_value());

  OrchardCheckpoint checkpoint1;
  checkpoint1.marks_removed = std::vector<uint32_t>({1, 2, 3});
  checkpoint1.tree_state_position = 4;
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                checkpoint1.Clone())
                .value());

  OrchardCheckpoint checkpoint_different_marks_removed;
  checkpoint_different_marks_removed.tree_state_position =
      checkpoint1.tree_state_position;
  checkpoint_different_marks_removed.marks_removed =
      std::vector<uint32_t>({1, 2});
  EXPECT_FALSE(orchard_storage_
                   ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                   checkpoint_different_marks_removed.Clone())
                   .has_value());

  OrchardCheckpoint checkpoint_different_position1;
  checkpoint_different_position1.marks_removed = checkpoint1.marks_removed;
  checkpoint_different_position1.tree_state_position = 7;
  EXPECT_FALSE(orchard_storage_
                   ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                   checkpoint_different_position1.Clone())
                   .has_value());

  OrchardCheckpoint checkpoint_different_position2;
  checkpoint_different_position2.tree_state_position = std::nullopt;
  checkpoint_different_position2.marks_removed = checkpoint1.marks_removed;
  EXPECT_FALSE(orchard_storage_
                   ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                   checkpoint_different_position2.Clone())
                   .has_value());

  EXPECT_EQ(
      OrchardCheckpointBundle(1, checkpoint1.Clone()),
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value()
          .value());
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

    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(OrchardPool::kOrchard, account_id, notes,
                                  spends, 200, "hash200")
                    .has_value());
  }

  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>();
    checkpoint.tree_state_position = std::nullopt;
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                  checkpoint.Clone())
                  .value());
  }

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->ResetAccountSyncState(account_id, std::nullopt)
                .value());

  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id);
    EXPECT_EQ(100u, account_meta.value()->account_birthday);
    EXPECT_FALSE(account_meta.value()->latest_scanned_block_id);
    EXPECT_FALSE(account_meta.value()->latest_scanned_block_hash);
  }

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->ResetAccountSyncState(account_id, 200u).value());
  {
    auto account_meta = orchard_storage_->GetAccountMeta(account_id);
    EXPECT_EQ(200u, account_meta.value()->account_birthday);
    EXPECT_FALSE(account_meta.value()->latest_scanned_block_id);
    EXPECT_FALSE(account_meta.value()->latest_scanned_block_hash);
  }

  EXPECT_EQ(0u, orchard_storage_
                    ->GetCheckpoints(OrchardPool::kOrchard, account_id, 100)
                    .value()
                    .size());
  EXPECT_EQ(
      0u, orchard_storage_->GetSpendableNotes(OrchardPool::kOrchard, account_id)
              .value()
              .size());
  EXPECT_EQ(0u,
            orchard_storage_->GetNullifiers(OrchardPool::kOrchard, account_id)
                .value()
                .size());
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
              orchard_storage_
                  ->AddCheckpoint(OrchardPool::kOrchard, account_id, 3,
                                  checkpoint.Clone())
                  .value());
  }

  // Update existing checkpoint.
  {
    OrchardCheckpoint checkpoint;
    checkpoint.marks_removed = std::vector<uint32_t>({1});
    checkpoint.tree_state_position = 15;
    EXPECT_EQ(
        OrchardStorage::Result::kSuccess,
        orchard_storage_
            ->UpdateCheckpoint(OrchardPool::kOrchard, account_id, 3, checkpoint)
            .value());
    auto get_checkpoint_result =
        orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 3);
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
        orchard_storage_
            ->UpdateCheckpoint(OrchardPool::kOrchard, account_id, 5, checkpoint)
            .value());
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

TEST_F(OrchardStorageTest, PoolIsolation_Notes) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RegisterAccount(account_id, 100).value());

  // Same nullifier may exist independently in each pool.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id, 101, 1));
    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(OrchardPool::kOrchard, account_id, notes, {},
                                  101, "hash101")
                    .has_value());
    notes.front().note_version = 3;
    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(OrchardPool::kIronwood, account_id, notes, {},
                                  101, "hash101")
                    .has_value());
  }

  {
    auto orchard_notes =
        orchard_storage_->GetSpendableNotes(OrchardPool::kOrchard, account_id);
    ASSERT_TRUE(orchard_notes.has_value());
    EXPECT_EQ(1u, orchard_notes->size());

    auto ironwood_notes =
        orchard_storage_->GetSpendableNotes(OrchardPool::kIronwood, account_id);
    ASSERT_TRUE(ironwood_notes.has_value());
    EXPECT_EQ(1u, ironwood_notes->size());
    EXPECT_EQ(orchard_notes->front().nullifier,
              ironwood_notes->front().nullifier);
  }

  // Duplicate nullifier within the same pool is rejected.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id, 102, 1));
    EXPECT_FALSE(orchard_storage_
                     ->UpdateNotes(OrchardPool::kOrchard, account_id, notes, {},
                                   102, "hash102")
                     .has_value());
  }

  // Orchard-only note is not visible to Ironwood.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(GenerateMockOrchardNote(account_id, 103, 2));
    EXPECT_TRUE(orchard_storage_
                    ->UpdateNotes(OrchardPool::kOrchard, account_id, notes, {},
                                  103, "hash103")
                    .has_value());
    EXPECT_EQ(2u, orchard_storage_
                      ->GetSpendableNotes(OrchardPool::kOrchard, account_id)
                      .value()
                      .size());
    EXPECT_EQ(1u, orchard_storage_
                      ->GetSpendableNotes(OrchardPool::kIronwood, account_id)
                      .value()
                      .size());
  }

  // Chain reorg affects notes in both pools.
  EXPECT_TRUE(orchard_storage_->HandleChainReorg(account_id, 100u, "hash100")
                  .has_value());
  EXPECT_EQ(
      0u, orchard_storage_->GetSpendableNotes(OrchardPool::kOrchard, account_id)
              .value()
              .size());
  EXPECT_EQ(0u, orchard_storage_
                    ->GetSpendableNotes(OrchardPool::kIronwood, account_id)
                    .value()
                    .size());
}

TEST_F(OrchardStorageTest, PoolIsolation_SpentNotes) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RegisterAccount(account_id, 100).value());

  auto nullifier = GenerateMockNullifier(account_id, 7);
  std::vector<OrchardNoteSpend> spends = {OrchardNoteSpend{150, nullifier}};

  EXPECT_TRUE(orchard_storage_
                  ->UpdateNotes(OrchardPool::kOrchard, account_id, {}, spends,
                                150, "hash150")
                  .has_value());
  EXPECT_TRUE(orchard_storage_
                  ->UpdateNotes(OrchardPool::kIronwood, account_id, {}, spends,
                                150, "hash150")
                  .has_value());

  {
    auto orchard_nullifiers =
        orchard_storage_->GetNullifiers(OrchardPool::kOrchard, account_id);
    ASSERT_TRUE(orchard_nullifiers.has_value());
    ASSERT_EQ(1u, orchard_nullifiers->size());
    EXPECT_EQ(nullifier, orchard_nullifiers->front().nullifier);

    auto ironwood_nullifiers =
        orchard_storage_->GetNullifiers(OrchardPool::kIronwood, account_id);
    ASSERT_TRUE(ironwood_nullifiers.has_value());
    ASSERT_EQ(1u, ironwood_nullifiers->size());
    EXPECT_EQ(nullifier, ironwood_nullifiers->front().nullifier);
  }

  // Duplicate spent nullifier within the same pool is rejected.
  EXPECT_FALSE(orchard_storage_
                   ->UpdateNotes(OrchardPool::kOrchard, account_id, {}, spends,
                                 151, "hash151")
                   .has_value());

  // Reorg clears spends in both pools.
  EXPECT_TRUE(orchard_storage_->HandleChainReorg(account_id, 100u, "hash100")
                  .has_value());
  EXPECT_EQ(0u,
            orchard_storage_->GetNullifiers(OrchardPool::kOrchard, account_id)
                .value()
                .size());
  EXPECT_EQ(0u,
            orchard_storage_->GetNullifiers(OrchardPool::kIronwood, account_id)
                .value()
                .size());
}

TEST_F(OrchardStorageTest, PoolIsolation_ShardTree) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RegisterAccount(account_id, 100).value());

  OrchardShard orchard_shard = CreateShard(/*index=*/3, /*level=*/1);
  orchard_shard.shard_data = std::vector<uint8_t>({1, 1, 1, 1});
  OrchardShard ironwood_shard = CreateShard(/*index=*/3, /*level=*/1);
  ironwood_shard.shard_data = std::vector<uint8_t>({2, 2, 2, 2});
  ironwood_shard.root_hash->fill(33);

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->PutShard(OrchardPool::kOrchard, account_id, orchard_shard)
                .value());
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->PutShard(OrchardPool::kIronwood, account_id, ironwood_shard)
                .value());

  {
    auto orchard_got = orchard_storage_->GetShard(
        OrchardPool::kOrchard, account_id, OrchardShardAddress{1, 3});
    ASSERT_TRUE(orchard_got.has_value());
    ASSERT_TRUE(orchard_got.value().has_value());
    EXPECT_EQ(orchard_shard, *orchard_got.value());

    auto ironwood_got = orchard_storage_->GetShard(
        OrchardPool::kIronwood, account_id, OrchardShardAddress{1, 3});
    ASSERT_TRUE(ironwood_got.has_value());
    ASSERT_TRUE(ironwood_got.value().has_value());
    EXPECT_EQ(ironwood_shard, *ironwood_got.value());
  }

  EXPECT_EQ(3u, orchard_storage_
                    ->GetLatestShardIndex(OrchardPool::kOrchard, account_id)
                    .value()
                    .value());
  EXPECT_EQ(3u, orchard_storage_
                    ->GetLatestShardIndex(OrchardPool::kIronwood, account_id)
                    .value()
                    .value());

  // Truncating one pool leaves the other pool's shard tree intact.
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->TruncateShards(OrchardPool::kOrchard, account_id, 3)
          .value());
  EXPECT_EQ(std::nullopt, orchard_storage_
                              ->GetShard(OrchardPool::kOrchard, account_id,
                                         OrchardShardAddress{1, 3})
                              .value());
  EXPECT_EQ(ironwood_shard, orchard_storage_
                                ->GetShard(OrchardPool::kIronwood, account_id,
                                           OrchardShardAddress{1, 3})
                                .value()
                                .value());
}

TEST_F(OrchardStorageTest, PoolIsolation_ShardTreeCap) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RegisterAccount(account_id, 100).value());

  OrchardShardTreeCap orchard_cap = {1, 2, 3};
  OrchardShardTreeCap ironwood_cap = {4, 5, 6};

  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->PutCap(OrchardPool::kOrchard, account_id, orchard_cap)
          .value());
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->PutCap(OrchardPool::kIronwood, account_id, ironwood_cap)
          .value());

  EXPECT_EQ(orchard_cap,
            orchard_storage_->GetCap(OrchardPool::kOrchard, account_id)
                .value()
                .value());
  EXPECT_EQ(ironwood_cap,
            orchard_storage_->GetCap(OrchardPool::kIronwood, account_id)
                .value()
                .value());

  // Updating one pool's cap does not overwrite the other.
  OrchardShardTreeCap updated_orchard_cap = {9, 9, 9};
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->PutCap(OrchardPool::kOrchard, account_id, updated_orchard_cap)
                .value());
  EXPECT_EQ(updated_orchard_cap,
            orchard_storage_->GetCap(OrchardPool::kOrchard, account_id)
                .value()
                .value());
  EXPECT_EQ(ironwood_cap,
            orchard_storage_->GetCap(OrchardPool::kIronwood, account_id)
                .value()
                .value());
}

TEST_F(OrchardStorageTest, PoolIsolation_Checkpoints) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RegisterAccount(account_id, 100).value());
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_->RegisterAccount(account_id_2, 100).value());

  OrchardCheckpoint orchard_cp;
  orchard_cp.marks_removed = {1, 2};
  orchard_cp.tree_state_position = 10;

  OrchardCheckpoint ironwood_cp;
  ironwood_cp.marks_removed = {3, 4};
  ironwood_cp.tree_state_position = 20;

  OrchardCheckpoint account2_cp;
  account2_cp.marks_removed = {5};
  account2_cp.tree_state_position = 30;

  // Same checkpoint_id is allowed across pools and accounts.
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id, 1,
                                orchard_cp.Clone())
                .value());
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kIronwood, account_id, 1,
                                ironwood_cp.Clone())
                .value());
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            orchard_storage_
                ->AddCheckpoint(OrchardPool::kOrchard, account_id_2, 1,
                                account2_cp.Clone())
                .value());

  {
    auto orchard_got =
        orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1);
    ASSERT_TRUE(orchard_got.has_value());
    ASSERT_TRUE(orchard_got.value().has_value());
    EXPECT_EQ(OrchardCheckpointBundle(1, orchard_cp.Clone()),
              orchard_got.value().value());

    auto ironwood_got =
        orchard_storage_->GetCheckpoint(OrchardPool::kIronwood, account_id, 1);
    ASSERT_TRUE(ironwood_got.has_value());
    ASSERT_TRUE(ironwood_got.value().has_value());
    EXPECT_EQ(OrchardCheckpointBundle(1, ironwood_cp.Clone()),
              ironwood_got.value().value());

    auto account2_got =
        orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id_2, 1);
    ASSERT_TRUE(account2_got.has_value());
    ASSERT_TRUE(account2_got.value().has_value());
    EXPECT_EQ(OrchardCheckpointBundle(1, account2_cp.Clone()),
              account2_got.value().value());
  }

  // checkpoints_mark_removed rows are also pool/account-scoped.
  EXPECT_EQ(
      std::vector<uint32_t>({1, 2}),
      orchard_storage_->GetMarksRemoved(OrchardPool::kOrchard, account_id, 1)
          .value());
  EXPECT_EQ(
      std::vector<uint32_t>({3, 4}),
      orchard_storage_->GetMarksRemoved(OrchardPool::kIronwood, account_id, 1)
          .value());
  EXPECT_EQ(
      std::vector<uint32_t>({5}),
      orchard_storage_->GetMarksRemoved(OrchardPool::kOrchard, account_id_2, 1)
          .value());

  // Mutations are scoped to a single pool + account.
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      orchard_storage_->RemoveCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value());
  EXPECT_EQ(
      std::nullopt,
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value());
  EXPECT_TRUE(
      orchard_storage_->GetCheckpoint(OrchardPool::kIronwood, account_id, 1)
          .value()
          .has_value());
  EXPECT_TRUE(
      orchard_storage_->GetCheckpoint(OrchardPool::kOrchard, account_id_2, 1)
          .value()
          .has_value());
}

TEST_F(OrchardStorageTest, MigrateV2ToV3) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);

  base::FilePath db_path = temp_dir_.GetPath().AppendASCII("orchard_v2.db");
  PopulateOrchardV2DatabaseForTesting(db_path, account_id);

  // Opening OrchardStorage triggers the v2→v3 migration.
  {
    OrchardStorage migrated_storage(db_path);
    ASSERT_TRUE(migrated_storage.EnsureDbInit());

    // account_meta is unchanged by migration (no pool-specific tables).
    {
      auto meta = migrated_storage.GetAccountMeta(account_id);
      ASSERT_TRUE(meta.has_value());
      ASSERT_TRUE(meta.value().has_value());
      EXPECT_EQ(meta.value()->account_birthday, 100u);
      EXPECT_EQ(meta.value()->latest_scanned_block_id, 105u);
    }

    // Note migrated under orchard tables.
    {
      auto notes =
          migrated_storage.GetSpendableNotes(OrchardPool::kOrchard, account_id);
      ASSERT_TRUE(notes.has_value());
      EXPECT_EQ(1u, notes->size());
    }

    // Checkpoint migrated under orchard tables.
    {
      auto min_id =
          migrated_storage.MinCheckpointId(OrchardPool::kOrchard, account_id);
      ASSERT_TRUE(min_id.has_value());
      EXPECT_EQ(1u, min_id->value());

      auto checkpoint =
          migrated_storage.GetCheckpoint(OrchardPool::kOrchard, account_id, 1);
      ASSERT_TRUE(checkpoint.has_value());
      ASSERT_TRUE(checkpoint.value().has_value());
      EXPECT_EQ(4, checkpoint.value()->checkpoint.tree_state_position);
      // checkpoints_mark_removed rows survive the checkpoints table rebuild.
      EXPECT_EQ(std::vector<uint32_t>({1, 2, 3}),
                checkpoint.value()->checkpoint.marks_removed);
    }

    // Ironwood tables are empty after migration.
    {
      auto notes = migrated_storage.GetSpendableNotes(OrchardPool::kIronwood,
                                                      account_id);
      ASSERT_TRUE(notes.has_value());
      EXPECT_EQ(0u, notes->size());
      auto min_id =
          migrated_storage.MinCheckpointId(OrchardPool::kIronwood, account_id);
      ASSERT_TRUE(min_id.has_value());
      EXPECT_EQ(std::nullopt, min_id.value());
    }

    // After migration, ironwood tables accept data independently of orchard
    // (including the same nullifier / checkpoint_id).
    {
      std::vector<OrchardNote> notes;
      notes.push_back(GenerateMockOrchardNote(account_id, 101, 1));
      // Reuse a nullifier that already exists under orchard after migrate.
      notes.front().nullifier.fill(1);
      notes.front().note_version = 3;
      EXPECT_TRUE(migrated_storage
                      .UpdateNotes(OrchardPool::kIronwood, account_id, notes,
                                   {}, 101, "hash101")
                      .has_value());

      OrchardCheckpoint cp;
      cp.marks_removed = {};
      cp.tree_state_position = 4;
      EXPECT_EQ(
          OrchardStorage::Result::kSuccess,
          migrated_storage
              .AddCheckpoint(OrchardPool::kIronwood, account_id, 1, cp.Clone())
              .value());
    }
  }

  // Inspect schema and note_version after OrchardStorage has closed the DB.
  {
    sql::Database database(sql::Database::Tag("MigrateV2ToV3Schema"));
    ASSERT_TRUE(database.Open(db_path));

    EXPECT_TRUE(database.DoesTableExist("orchard_tree_retained_checkpoints"));
    EXPECT_FALSE(database.DoesTableExist("ironwood_tree_retained_checkpoints"));

    sql::Statement orchard_note_version(database.GetUniqueStatement(
        "SELECT note_version FROM notes WHERE account_id = ?;"));
    orchard_note_version.BindString(0, account_id->unique_key);
    ASSERT_TRUE(orchard_note_version.Step());
    EXPECT_EQ(2, orchard_note_version.ColumnInt(0));

    sql::Statement ironwood_note_version(database.GetUniqueStatement(
        "SELECT note_version FROM notes_ironwood WHERE account_id = ?;"));
    ironwood_note_version.BindString(0, account_id->unique_key);
    ASSERT_TRUE(ironwood_note_version.Step());
    EXPECT_EQ(3, ironwood_note_version.ColumnInt(0));
  }
}

// v2 databases have no retained-checkpoints table. Migration must create
// orchard_tree_retained_checkpoints and the table must be usable afterward.
TEST_F(OrchardStorageTest, MigrateV2ToV3_OrchardRetainedCheckpoints) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);

  base::FilePath db_path =
      temp_dir_.GetPath().AppendASCII("orchard_v2_retained.db");
  PopulateOrchardV2DatabaseForTesting(db_path, account_id);

  {
    sql::Database database(sql::Database::Tag("MigrateV2ToV3RetainedBefore"));
    ASSERT_TRUE(database.Open(db_path));
    EXPECT_FALSE(database.DoesTableExist("orchard_tree_retained_checkpoints"));
  }

  {
    OrchardStorage migrated_storage(db_path);
    ASSERT_TRUE(migrated_storage.EnsureDbInit());

    {
      auto retained = migrated_storage.GetOrchardRetainedCheckpoints();
      ASSERT_TRUE(retained.has_value());
      EXPECT_TRUE(retained->empty());
    }

    {
      auto tx = migrated_storage.Transactionally();
      ASSERT_TRUE(tx.has_value());
      EXPECT_EQ(OrchardStorage::Result::kSuccess,
                migrated_storage.AddOrchardRetainedCheckpoint(100).value());
      EXPECT_EQ(OrchardStorage::Result::kSuccess,
                migrated_storage.AddOrchardRetainedCheckpoint(200).value());
      EXPECT_EQ(OrchardStorage::Result::kSuccess, tx->Commit().value());
    }

    {
      auto retained = migrated_storage.GetOrchardRetainedCheckpoints();
      ASSERT_TRUE(retained.has_value());
      EXPECT_EQ(std::vector<uint32_t>({100, 200}), retained.value());
    }

    {
      auto tx = migrated_storage.Transactionally();
      ASSERT_TRUE(tx.has_value());
      EXPECT_EQ(OrchardStorage::Result::kSuccess,
                migrated_storage.RemoveOrchardRetainedCheckpoint(100).value());
      EXPECT_EQ(OrchardStorage::Result::kSuccess, tx->Commit().value());
    }

    {
      auto retained = migrated_storage.GetOrchardRetainedCheckpoints();
      ASSERT_TRUE(retained.has_value());
      EXPECT_EQ(std::vector<uint32_t>({200}), retained.value());
    }
  }

  {
    sql::Database database(sql::Database::Tag("MigrateV2ToV3RetainedAfter"));
    ASSERT_TRUE(database.Open(db_path));
    EXPECT_TRUE(database.DoesTableExist("orchard_tree_retained_checkpoints"));
    EXPECT_FALSE(database.DoesTableExist("ironwood_tree_retained_checkpoints"));
  }
}

// Regression test: v2's "checkpoints" table used `checkpoint_id` as a
// standalone PRIMARY KEY, so a checkpoint_id could only ever be used by a
// single account across the whole table. Verify that after migrating to v3,
// the rebuilt table uses a composite (account_id, checkpoint_id) primary key
// so a second account can reuse the same checkpoint_id.
TEST_F(OrchardStorageTest, MigrateV2ToV3_CheckpointsPrimaryKeyIsPerAccount) {
  auto account_id = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                            mojom::KeyringId::kZCashMainnet,
                                            mojom::AccountKind::kDerived, 0);
  auto account_id_2 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 1);

  base::FilePath db_path = temp_dir_.GetPath().AppendASCII("orchard_v2.db");
  // account_id already owns checkpoint_id 1 in the v2 database. Under the
  // old (buggy) schema, no other account could ever add a checkpoint with
  // id 1 without hitting a PRIMARY KEY constraint violation.
  PopulateOrchardV2DatabaseForTesting(db_path, account_id);

  OrchardStorage migrated_storage(db_path);
  ASSERT_TRUE(migrated_storage.EnsureDbInit());

  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            migrated_storage.RegisterAccount(account_id_2, 100).value());

  OrchardCheckpoint checkpoint_for_account_2;
  checkpoint_for_account_2.marks_removed = std::vector<uint32_t>({9});
  checkpoint_for_account_2.tree_state_position = 42;

  // Adding checkpoint_id 1 for a different account must succeed now that the
  // primary key is (account_id, checkpoint_id).
  EXPECT_EQ(OrchardStorage::Result::kSuccess,
            migrated_storage
                .AddCheckpoint(OrchardPool::kOrchard, account_id_2, 1,
                               checkpoint_for_account_2.Clone())
                .value());

  // Both accounts' checkpoint_id 1 rows coexist and are independently
  // retrievable.
  {
    auto account_1_checkpoint =
        migrated_storage.GetCheckpoint(OrchardPool::kOrchard, account_id, 1);
    ASSERT_TRUE(account_1_checkpoint.has_value());
    ASSERT_TRUE(account_1_checkpoint.value().has_value());
    EXPECT_EQ(4, account_1_checkpoint.value()->checkpoint.tree_state_position);
    EXPECT_EQ(std::vector<uint32_t>({1, 2, 3}),
              account_1_checkpoint.value()->checkpoint.marks_removed);

    auto account_2_checkpoint =
        migrated_storage.GetCheckpoint(OrchardPool::kOrchard, account_id_2, 1);
    ASSERT_TRUE(account_2_checkpoint.has_value());
    ASSERT_TRUE(account_2_checkpoint.value().has_value());
    EXPECT_EQ(OrchardCheckpointBundle(1, checkpoint_for_account_2.Clone()),
              account_2_checkpoint.value().value());
  }

  // Removing account_1's checkpoint doesn't affect account_2's.
  EXPECT_EQ(
      OrchardStorage::Result::kSuccess,
      migrated_storage.RemoveCheckpoint(OrchardPool::kOrchard, account_id, 1)
          .value());
  EXPECT_EQ(std::nullopt,
            migrated_storage.GetCheckpoint(OrchardPool::kOrchard, account_id, 1)
                .value());
  EXPECT_TRUE(
      migrated_storage.GetCheckpoint(OrchardPool::kOrchard, account_id_2, 1)
          .value()
          .has_value());
}

TEST_F(OrchardStorageTest, OrchardRetainedCheckpoints) {
  {
    auto retained = orchard_storage_->GetOrchardRetainedCheckpoints();
    ASSERT_TRUE(retained.has_value());
    EXPECT_TRUE(retained->empty());
  }

  {
    auto tx = orchard_storage_->Transactionally();
    ASSERT_TRUE(tx.has_value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddOrchardRetainedCheckpoint(100).value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddOrchardRetainedCheckpoint(200).value());
    // INSERT OR IGNORE: adding the same checkpoint again is a no-op success.
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->AddOrchardRetainedCheckpoint(100).value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess, tx->Commit().value());
  }

  {
    auto retained = orchard_storage_->GetOrchardRetainedCheckpoints();
    ASSERT_TRUE(retained.has_value());
    EXPECT_EQ(std::vector<uint32_t>({100, 200}), retained.value());
  }

  {
    auto tx = orchard_storage_->Transactionally();
    ASSERT_TRUE(tx.has_value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess,
              orchard_storage_->RemoveOrchardRetainedCheckpoint(100).value());
    EXPECT_EQ(OrchardStorage::Result::kSuccess, tx->Commit().value());
  }

  {
    auto retained = orchard_storage_->GetOrchardRetainedCheckpoints();
    ASSERT_TRUE(retained.has_value());
    EXPECT_EQ(std::vector<uint32_t>({200}), retained.value());
  }
}

}  // namespace brave_wallet
