/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/contribution/contribution.h"
#include "bat/ledger/internal/logging.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/publisher_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ContributionTest.*

namespace bat_ledger {

class MockLedgerImpl : public LedgerImpl {
 public:
  MockLedgerImpl() {}
  MOCK_METHOD1(IsPublisherConnectedOrVerified,
      void(const ledger::PublisherStatus status));
};

}

namespace braveledger_contribution {

class ContributionTest : public testing::Test {
 protected:
  void GetPublishersForAuto(
      ledger::PublisherInfoList* publisher_info_list,
      uint32_t iterations,  /* total count of publishers */
      uint32_t variation  /* total count of verifieds */) {
    // Can't have more verified publishers than total publishers
    DCHECK(variation <= iterations);
    for (uint32_t ix = 0; ix < iterations; ix++) {
      const auto status =
          ix < variation
          ? ledger::PublisherStatus::VERIFIED
          : ledger::PublisherStatus::NOT_VERIFIED;
      ledger::PublisherInfoPtr publisher_info = ledger::PublisherInfo::New();
      publisher_info->id = "example" + std::to_string(ix) + ".com";
      publisher_info->verified = status;
      publisher_info->percent = (1.0 / iterations) * 100.0;
      publisher_info->weight = (1.0 / iterations) * 100.0;
      publisher_info_list->push_back(std::move(publisher_info));
    }
  }

  void GetPublishersForRecurring(
      ledger::PublisherInfoList* publisher_info_list,
      uint32_t iterations,
      std::vector<uint32_t> amounts,
      uint32_t variation) {
    for (uint32_t ix = 0; ix < iterations; ix++) {
      ledger::PublisherInfoPtr publisher_info = ledger::PublisherInfo::New();
      publisher_info->id = "recurringexample" + std::to_string(ix) + ".com";
      publisher_info->weight = amounts[ix % amounts.size()];
      publisher_info->verified = ix < variation;
      publisher_info_list->push_back(std::move(publisher_info));
    }
  }

  bool WillTriggerNotification(
      uint32_t auto_iterations,
      uint32_t auto_variations,
      double auto_amount_selected,
      uint32_t recurring_iterations,
      std::vector<uint32_t> recurring_amounts_selected,
      uint32_t recurring_variation,
      double wallet_balance) {
    ledger::PublisherInfoList publisher_info_list_auto;
    ledger::PublisherInfoList publisher_info_list_recurring;
    GetPublishersForAuto(
        &publisher_info_list_auto, auto_iterations, auto_variations);
    double total_reconcile_amount =
        Contribution::GetAmountFromVerifiedAuto(
          publisher_info_list_auto, auto_amount_selected);
    GetPublishersForRecurring(
        &publisher_info_list_recurring,
        recurring_iterations,
        recurring_amounts_selected,
        recurring_variation);
    total_reconcile_amount +=
        Contribution::GetAmountFromVerifiedRecurring(
          publisher_info_list_recurring);
    return wallet_balance < total_reconcile_amount &&
        !publisher_info_list_auto.empty() &&
        !publisher_info_list_recurring.empty();
  }
};

TEST_F(ContributionTest, GetAmountFromVerifiedAuto) {
  auto contribution =
      std::make_unique<braveledger_contribution::ContributionTest>(nullptr);
  ledger::PublisherInfoList publisher_info_list;

  // 0 publishers and budget of 0 BAT
  GetPublishersForAuto(&publisher_info_list, 0, 0);
  double amount =
      contribution->GetAmountFromVerifiedAuto(publisher_info_list, 0);
  EXPECT_EQ(amount, 0);

  // 10 publishers total with 5 verified and budget of 30 BAT
  GetPublishersForAuto(&publisher_info_list, 10, 5);
  amount =
      contribution->GetAmountFromVerifiedAuto(publisher_info_list, 30);
  EXPECT_EQ(amount, 15);

  // 20 publishers total with 10 verified and budget of 30 BAT
  publisher_info_list.clear();
  GetPublishersForAuto(&publisher_info_list, 20, 10);
  amount =
      contribution->GetAmountFromVerifiedAuto(publisher_info_list, 30);
  EXPECT_EQ(amount, 15);

  // 50 publishers total with 5 verified and budget of 100 BAT
  publisher_info_list.clear();
  GetPublishersForAuto(&publisher_info_list, 50, 5);
  amount =
      contribution->GetAmountFromVerifiedAuto(publisher_info_list, 100);
  EXPECT_EQ(amount, 10);

  // 100 publishers total with 80 verified and budget of 1478 BAT
  publisher_info_list.clear();
  GetPublishersForAuto(&publisher_info_list, 100, 80);
  amount =
      contribution->GetAmountFromVerifiedAuto(publisher_info_list, 1478);
  EXPECT_NEAR(amount, 1182.4, 0.001f);

  // 100 publishers total with 4 verified and budget of 100 BAT
  publisher_info_list.clear();
  GetPublishersForAuto(&publisher_info_list, 100, 4);
  amount =
      contribution->GetAmountFromVerifiedAuto(publisher_info_list, 100);
  EXPECT_EQ(amount, 4);
}

TEST_F(ContributionTest, GetAmountFromVerifiedRecurring) {
  ledger::PublisherInfoList publisher_info_list;
  GetPublishersForRecurring(&publisher_info_list, 5, {1, 5, 10}, 2);
  double amount =
      Contribution::GetAmountFromVerifiedRecurring(publisher_info_list);
  EXPECT_EQ(amount, 6);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 7, {1, 5, 10}, 5);
  amount =
      Contribution::GetAmountFromVerifiedRecurring(publisher_info_list);
  EXPECT_EQ(amount, 22);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 10, {5, 10, 20}, 7);
  amount =
      Contribution::GetAmountFromVerifiedRecurring(publisher_info_list);
  EXPECT_EQ(amount, 75);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 10, {10, 20, 50}, 9);
  amount =
      Contribution::GetAmountFromVerifiedRecurring(publisher_info_list);
  EXPECT_EQ(amount, 240);

  publisher_info_list.clear();
  GetPublishersForRecurring(&publisher_info_list, 5, {1, 5, 10, 20, 50}, 5);
  amount =
      Contribution::GetAmountFromVerifiedRecurring(publisher_info_list);
  EXPECT_EQ(amount, 86);
}

TEST_F(ContributionTest, WillTriggerNotification) {
  // 0 auto, 0 tips, 0 balance
  EXPECT_FALSE(WillTriggerNotification(0, 0, 20, 0, {1, 5, 10}, 0, 0.0));

  EXPECT_TRUE(WillTriggerNotification(10, 5, 30, 5, {1, 5, 10}, 2, 20.9));
  EXPECT_FALSE(WillTriggerNotification(10, 5, 30, 5, {1, 5, 10}, 2, 21));
  EXPECT_TRUE(WillTriggerNotification(20, 10, 30, 7, {1, 5, 10}, 5, 36.9));
  EXPECT_FALSE(WillTriggerNotification(20, 10, 30, 7, {1, 5, 10}, 5, 37));
  EXPECT_TRUE(WillTriggerNotification(50, 5, 100, 10, {5, 10, 20}, 7, 84.9));
  EXPECT_FALSE(WillTriggerNotification(50, 5, 100, 10, {5, 10, 20}, 7, 85));
  EXPECT_TRUE(WillTriggerNotification(
      100, 80, 1478, 10, {10, 20, 50}, 9, 1422.39));
  EXPECT_FALSE(WillTriggerNotification(
      100, 80, 1478, 10, {10, 20, 50}, 9, 1422.40));
  EXPECT_TRUE(WillTriggerNotification(
      100, 4, 100, 5, {1, 5, 10, 20, 50}, 5, 89.9));
  EXPECT_FALSE(WillTriggerNotification(
      100, 4, 100, 5, {1, 5, 10, 20, 50}, 5, 90));
}

}  // namespace braveledger_contribution
