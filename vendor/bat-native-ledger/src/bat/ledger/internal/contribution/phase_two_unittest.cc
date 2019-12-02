/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "bat/ledger/internal/contribution/phase_two.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/properties/ballot_properties.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PhaseTwoTest.*

namespace braveledger_contribution {

class PhaseTwoTest : public testing::Test {
 protected:
  void PopulateDirectionsList(ledger::ReconcileDirections* list) {
    ledger::ReconcileDirectionProperties publisher;

    publisher.publisher_key = "publisher1";
    publisher.amount_percent = 2.0;
    list->push_back(publisher);

    publisher.publisher_key = "publisher2";
    publisher.amount_percent = 13.0;
    list->push_back(publisher);

    publisher.publisher_key = "publisher3";
    publisher.amount_percent = 14.0;
    list->push_back(publisher);

    publisher.publisher_key = "publisher4";
    publisher.amount_percent = 23.0;
    list->push_back(publisher);

    publisher.publisher_key = "publisher5";
    publisher.amount_percent = 38.0;
    list->push_back(publisher);
  }
};

TEST_F(PhaseTwoTest, GetStatisticalVotingWinners) {
  auto phase_two =
      std::make_unique<braveledger_contribution::PhaseTwo>(nullptr, nullptr);

  ledger::ReconcileDirections list;
  PopulateDirectionsList(&list);

  struct {
    double dart;
    const char* publisher;
  } cases[] = {
      {0.01, "publisher1"},
      {0.05, "publisher2"},
      {0.10, "publisher2"},
      {0.20, "publisher3"},
      {0.30, "publisher4"},
      {0.40, "publisher4"},
      {0.50, "publisher4"},
      {0.60, "publisher5"},
      {0.70, "publisher5"},
      {0.80, "publisher5"},
      {0.90, "publisher5"},
  };

  for (size_t i = 0; i < base::size(cases); i++) {
    ledger::WinnerProperties winner;
    bool result = phase_two->GetStatisticalVotingWinner(
        cases[i].dart, list, &winner);
    EXPECT_TRUE(result);
    EXPECT_STREQ(winner.direction.publisher_key.c_str(), cases[i].publisher);
  }
}

// Surveyor IDs are not unique and may be shared between different transactions.
// Ensure that when assigning prepareBallot objects to ballots, we only assign
// to ballots for the current viewing ID, even if they share a surveyor ID.
TEST_F(PhaseTwoTest, AssignPrepareBallotsRespectsViewingID) {
  const std::string shared_surveyor_id =
      "Ad5pNzrwhWokTOR8/hC83LWJfEy8aY7mFwPQWe6CpRF";
  const std::vector<std::string> surveyors = {
      "{\"surveyorId\":\"" + shared_surveyor_id + "\"}"
  };

  // Create ballots with different viewing IDs but the same surveyor ID.
  ledger::Ballots ballots(2);
  ballots[0].viewing_id = "00000000-0000-0000-0000-000000000000";
  ballots[0].surveyor_id = shared_surveyor_id;
  ballots[1].viewing_id = "ffffffff-ffff-ffff-ffff-ffffffffffff";
  ballots[1].surveyor_id = shared_surveyor_id;

  // Check that only ballot[0] with the matching viewing ID is updated. Ballot 1
  // should remain unmodified.
  PhaseTwo::AssignPrepareBallots("00000000-0000-0000-0000-000000000000",
      surveyors, &ballots);
  ASSERT_FALSE(ballots[0].prepare_ballot.empty());
  ASSERT_TRUE(ballots[1].prepare_ballot.empty());
}

// Surveyor IDs may be reused between transactions. Ensure that proofs for
// ballots for one viewing ID will not be assigned to ballots for another
// viewing ID, even if they share a surveyor ID.
TEST_F(PhaseTwoTest, AssignProofsRespectsViewingID) {
  const std::vector<std::string> proofs = { "proof 1", "proof 2" };
  const std::string shared_surveyor_id =
      "Ad5pNzrwhWokTOR8/hC83LWJfEy8aY7mFwPQWe6CpRF";

  ledger::Ballots ballots(2);
  ballots[0].viewing_id = "00000000-0000-0000-0000-000000000000";
  ballots[0].surveyor_id = shared_surveyor_id;
  ballots[1].viewing_id = "ffffffff-ffff-ffff-ffff-ffffffffffff";
  ballots[1].surveyor_id = shared_surveyor_id;

  ledger::BatchProofs batch_proofs(2);
  batch_proofs[0].ballot = ballots[0];
  batch_proofs[1].ballot = ballots[1];

  PhaseTwo::AssignProofs(batch_proofs, proofs, &ballots);
  ASSERT_EQ(ballots[0].proof_ballot, proofs[0]);
  ASSERT_EQ(ballots[1].proof_ballot, proofs[1]);
}

}  // namespace braveledger_contribution
