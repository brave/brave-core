/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDaypartExclusionRuleTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    AdvanceClockTo(TimeFromString("Sun, 19 Mar 2023 05:35",
                                  /*is_local=*/true));  // Hello Rory!!!
  }

  const DaypartExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsDaypartExclusionRuleTest, ShouldIncludeIfNoDayparts) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDaypartExclusionRuleTest,
       ShouldIncludeIfMatchesDayOfWeekAndTimeSlot) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  CreativeDaypartInfo daypart;
  daypart.days_of_week = "0";  // Sunday
  daypart.start_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/35);
  daypart.end_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/35);
  creative_ad.dayparts.push_back(daypart);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDaypartExclusionRuleTest,
       ShouldIncludeIfMatchesDayOfWeekAndTimeSlotWhenMultipleDaysOfWeek) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  CreativeDaypartInfo daypart;
  daypart.days_of_week = "0123456";
  daypart.start_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/35);
  daypart.end_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/35);
  creative_ad.dayparts.push_back(daypart);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDaypartExclusionRuleTest,
       ShouldIncludeIfMatchesDayOfWeekAndTimeSlotWhenMultipleTimeSlots) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  CreativeDaypartInfo daypart_1;
  daypart_1.days_of_week = "1";  // Monday
  daypart_1.start_minute = test::GetMinutes(/*hours=*/0, /*minutes=*/0);
  daypart_1.end_minute = test::GetMinutes(/*hours=*/4, /*minutes=*/35);
  creative_ad.dayparts.push_back(daypart_1);

  CreativeDaypartInfo daypart_2;
  daypart_2.days_of_week = "23";  // Tuesday and Wednesday
  daypart_2.start_minute = test::GetMinutes(/*hours=*/4, /*minutes=*/36);
  daypart_2.end_minute = test::GetMinutes(/*hours=*/23, /*minutes=*/59);
  creative_ad.dayparts.push_back(daypart_2);

  CreativeDaypartInfo daypart_3;
  daypart_3.days_of_week = "0";  // Sunday
  daypart_3.start_minute = test::GetMinutes(/*hours=*/0, /*minutes=*/0);
  daypart_3.end_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/35);
  creative_ad.dayparts.push_back(daypart_3);

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDaypartExclusionRuleTest,
       DisallowWhenMatchingDayOfWeekButOutsideTimeSlot) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  CreativeDaypartInfo daypart_1;
  daypart_1.days_of_week = "1";  // Monday
  daypart_1.start_minute = test::GetMinutes(/*hours=*/0, /*minutes=*/0);
  daypart_1.end_minute = test::GetMinutes(/*hours=*/4, /*minutes=*/35);
  creative_ad.dayparts.push_back(daypart_1);

  CreativeDaypartInfo daypart_2;
  daypart_2.days_of_week = "0";  // Sunday
  daypart_2.start_minute = test::GetMinutes(/*hours=*/0, /*minutes=*/0);
  daypart_2.end_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/34);
  creative_ad.dayparts.push_back(daypart_2);

  CreativeDaypartInfo daypart_3;
  daypart_3.days_of_week = "0";  // Sunday
  daypart_3.start_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/36);
  daypart_3.end_minute = test::GetMinutes(/*hours=*/23, /*minutes=*/59);
  creative_ad.dayparts.push_back(daypart_3);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDaypartExclusionRuleTest, DisallowForWrongDayOfWeek) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  CreativeDaypartInfo daypart;
  daypart.days_of_week = "2";  // Tuesday
  daypart.start_minute = test::GetMinutes(/*hours=*/0, /*minutes=*/0);
  daypart.end_minute = test::GetMinutes(/*hours=*/23, /*minutes=*/59);
  creative_ad.dayparts.push_back(daypart);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsDaypartExclusionRuleTest,
       DisallowWhenMatchingDayOfWeekButOutsideCuspOfTimeSlot) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  CreativeDaypartInfo daypart_1;
  daypart_1.days_of_week = "0";  // Sunday
  daypart_1.start_minute = test::GetMinutes(/*hours=*/0, /*minutes=*/0);
  daypart_1.end_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/34);
  creative_ad.dayparts.push_back(daypart_1);

  CreativeDaypartInfo daypart_2;
  daypart_2.days_of_week = "0";  // Sunday
  daypart_2.start_minute = test::GetMinutes(/*hours=*/5, /*minutes=*/36);
  daypart_2.end_minute = test::GetMinutes(/*hours=*/23, /*minutes=*/59);
  creative_ad.dayparts.push_back(daypart_2);

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
