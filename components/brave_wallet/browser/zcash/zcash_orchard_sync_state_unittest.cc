/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_sync_state.h"

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bunde.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {

OrchardCommitmentValue CreateCommitmentValue(uint8_t leaf_index, uint8_t seed) {
  OrchardCommitmentValue value = {};
  value[0] = leaf_index;
  value[1] = seed;
}

OrchardCommitment CreateCommitment(OrchardCommitmentValue value,
                                   bool marked,
                                   std::optional<uint32_t> checkpoint_id) {
  return OrchardCommitment{value, marked, checkpoint_id};
}

}  // namespace

class ZCashOrchardSyncStateTest : public testing::Test {
 public:
  ZCashOrchardSyncStateTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  void SetUp() override;

  ZCashOrchardSyncState* sync_state() { return sync_state_.get(); }

 private:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<ZCashOrchardSyncState> sync_state_;
};

void ZCashOrchardSyncStateTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath db_path(
      temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
  sync_state_ = base::WrapRefCounted(new ZCashOrchardSyncState(db_path));
}

// Insert commitments without subtree roots
// Calculate tree state
TEST_F(ZCashOrchardSyncStateTest, InsertCommitmentsNoGap) {
  auto account_id_1 = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                              mojom::KeyringId::kZCashMainnet,
                                              mojom::AccountKind::kDerived, 0);
  EXPECT_TRUE(orchard_storage_->RegisterAccount(account_id_1.Clone(), 0, "hash")
                  .has_value());

  std::vector<OrchardCommitment> commitments;
  for (int i = 0; i < 4; i++) {
    commitments.push_back(
        CreateCommitment(CreateCommitmentValue(i, 0), false, std::nullopt));
  }
  // We have a note on 5
  commitments.push_back(
      CreateCommitment(CreateCommitmentValue(5, 0), true, std::nullopt));
  // We have checkpoint position with checkpoint_id = block_id = 1
  commitments.push_back(CreateCommitment(CreateCommitmentValue(6, 0), false,
                                         1 /* checkpoint id */));
  commitments.push_back(
      CreateCommitment(CreateCommitmentValue(7, 0), false, std::nullopt));
  commitments.push_back(
      CreateCommitment(CreateCommitmentValue(8, 0), false, std::nullopt));

  auto result =
      OrchardBlockScanner::CreateResultForTesting(std::nullopt, commitments);
  sync_state().UpdateNotes(account_id, result, 2, "2");
}

// Insert 2 subtree roots + 2 batches
TEST_F(ZCashOrchardSyncStateTest, SubtreeRoots) {}

// Insert 4 subtree roots
// Calculate tree state
TEST_F(ZCashOrchardSyncStateTest, SubtreeRoots) {}

}  // namespace brave_wallet
