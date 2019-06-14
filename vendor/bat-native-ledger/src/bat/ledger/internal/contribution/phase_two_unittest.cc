/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/contribution/phase_two.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PhaseTwoTest.*

namespace braveledger_contribution {

class PhaseTwoTest : public testing::Test {
 protected:
  void PopulatePublisherList(braveledger_bat_helper::PublisherList* list) {
    braveledger_bat_helper::PUBLISHER_ST publisher;

    publisher.id_ = "publisher1";
    publisher.weight_ = 2.0;
    list->push_back(publisher);

    publisher.id_ = "publisher2";
    publisher.weight_ = 13.0;
    list->push_back(publisher);

    publisher.id_ = "publisher3";
    publisher.weight_ = 14.0;
    list->push_back(publisher);

    publisher.id_ = "publisher4";
    publisher.weight_ = 23.0;
    list->push_back(publisher);

    publisher.id_ = "publisher5";
    publisher.weight_ = 38.0;
    list->push_back(publisher);
  }
};

TEST_F(PhaseTwoTest, GetStatisticalVotingWinners) {
  auto phase_two =
      std::make_unique<braveledger_contribution::PhaseTwo>(nullptr, nullptr);

  braveledger_bat_helper::PublisherList publisher_list;
  PopulatePublisherList(&publisher_list);

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
    braveledger_bat_helper::WINNERS_ST winner;
    bool result = phase_two->GetStatisticalVotingWinner(
        cases[i].dart, publisher_list, &winner);
    EXPECT_TRUE(result);
    EXPECT_STREQ(winner.publisher_data_.id_.c_str(), cases[i].publisher);
  }
}

}  // namespace braveledger_contribution
