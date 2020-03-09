/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ContributionUtilTest.*

namespace braveledger_contribution {

class ContributionUtilTest : public testing::Test {
 protected:
  void GetPublishersForRecurring(
      ledger::PublisherInfoList* publisher_info_list,
      uint32_t iterations,
      std::vector<uint32_t> amounts,
      uint32_t variation) {
    for (uint32_t ix = 0; ix < iterations; ix++) {
      const auto status =
          ix < variation
          ? ledger::PublisherStatus::VERIFIED
          : ledger::PublisherStatus::NOT_VERIFIED;
      ledger::PublisherInfoPtr publisher_info = ledger::PublisherInfo::New();
      publisher_info->id = "recurringexample" + std::to_string(ix) + ".com";
      publisher_info->weight = amounts[ix % amounts.size()];
      publisher_info->status = status;
      publisher_info_list->push_back(std::move(publisher_info));
    }
  }
};

TEST_F(ContributionUtilTest, GetTotalFromRecurringVerified) {
  ledger::PublisherInfoList publisher_info_list;
  GetPublishersForRecurring(&publisher_info_list, 5, {1, 5, 10}, 2);
  double amount = GetTotalFromRecurringVerified(publisher_info_list);
  EXPECT_EQ(amount, 6);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 7, {1, 5, 10}, 5);
  amount = GetTotalFromRecurringVerified(publisher_info_list);
  EXPECT_EQ(amount, 22);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 10, {5, 10, 20}, 7);
  amount = GetTotalFromRecurringVerified(publisher_info_list);
  EXPECT_EQ(amount, 75);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 10, {10, 20, 50}, 9);
  amount = GetTotalFromRecurringVerified(publisher_info_list);
  EXPECT_EQ(amount, 240);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 5, {1, 5, 10, 20, 50}, 5);
  amount = GetTotalFromRecurringVerified(publisher_info_list);
  EXPECT_EQ(amount, 86);
}

}  // namespace braveledger_contribution
