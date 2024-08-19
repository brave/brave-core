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

class OrchardStorageTest : public testing::Test {
 public:
  OrchardStorageTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override;

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<ZCashOrchardStorage> orchard_storage_;
};

void OrchardStorageTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
  orchard_storage_ = std::make_unique<ZCashOrchardStorage>(db_path);
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

}  // namespace brave_wallet
