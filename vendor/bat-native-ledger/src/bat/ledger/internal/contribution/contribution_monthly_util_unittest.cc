/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "bat/ledger/internal/contribution/contribution_monthly_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ContributionMonthlyUtilTest.*

namespace ledger {
namespace contribution {

class ContributionMonthlyUtilTest : public testing::Test {
 protected:
  void GetPublishersForRecurring(
      type::PublisherInfoList* publisher_info_list,
      uint32_t iterations,
      std::vector<uint32_t> amounts,
      uint32_t variation) {
    for (uint32_t ix = 0; ix < iterations; ix++) {
      const auto status = ix < variation
                              ? type::PublisherStatus::UPHOLD_VERIFIED
                              : type::PublisherStatus::NOT_VERIFIED;
      type::PublisherInfoPtr publisher_info = type::PublisherInfo::New();
      publisher_info->id = "recurringexample" + std::to_string(ix) + ".com";
      publisher_info->weight = amounts[ix % amounts.size()];
      publisher_info->status = status;
      publisher_info_list->push_back(std::move(publisher_info));
    }
  }
};

TEST_F(ContributionMonthlyUtilTest, GetTotalFromVerifiedTips) {
  type::PublisherInfoList publisher_info_list;
  GetPublishersForRecurring(&publisher_info_list, 5, {1, 5, 10}, 2);
  double amount = GetTotalFromVerifiedTips(publisher_info_list);
  EXPECT_EQ(amount, 6);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 7, {1, 5, 10}, 5);
  amount = GetTotalFromVerifiedTips(publisher_info_list);
  EXPECT_EQ(amount, 22);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 10, {5, 10, 20}, 7);
  amount = GetTotalFromVerifiedTips(publisher_info_list);
  EXPECT_EQ(amount, 75);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 10, {10, 20, 50}, 9);
  amount = GetTotalFromVerifiedTips(publisher_info_list);
  EXPECT_EQ(amount, 240);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 5, {1, 5, 10, 20, 50}, 5);
  amount = GetTotalFromVerifiedTips(publisher_info_list);
  EXPECT_EQ(amount, 86);
}

}  // namespace contribution
}  // namespace ledger
