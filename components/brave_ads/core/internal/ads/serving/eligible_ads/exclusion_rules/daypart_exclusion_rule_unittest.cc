/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsDaypartExclusionRuleTest : public UnitTestBase {
 protected:
  DaypartExclusionRule exclusion_rule_;
};

TEST_F(BatAdsDaypartExclusionRuleTest, AllowIfDaypartsIsEmpty) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_.ShouldExclude(creative_ad));
}

TEST_F(BatAdsDaypartExclusionRuleTest, AllowIfRightDayAndHours) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;

  CreativeDaypartInfo daypart_1;
  daypart_1.dow = base::NumberToString(exploded.day_of_week);
  daypart_1.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_1.end_minute = current_time + base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_1);

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_.ShouldExclude(creative_ad));
}

TEST_F(BatAdsDaypartExclusionRuleTest, AllowForMultipleDays) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;

  CreativeDaypartInfo daypart_1;
  daypart_1.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_1.end_minute = current_time + base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_1);

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_.ShouldExclude(creative_ad));
}

TEST_F(BatAdsDaypartExclusionRuleTest, AllowIfOneMatchExists) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;
  const std::string tomorrow_dow =
      base::NumberToString((exploded.day_of_week + 1) % 7);
  const std::string current_dow = base::NumberToString(exploded.day_of_week);

  CreativeDaypartInfo daypart_1;
  daypart_1.dow = tomorrow_dow;
  daypart_1.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_1.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_1);

  CreativeDaypartInfo daypart_2;
  daypart_2.dow = tomorrow_dow;
  daypart_2.start_minute = current_time + 2 * base::Time::kMinutesPerHour;
  daypart_2.end_minute = current_time + 3 * base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_2);

  CreativeDaypartInfo daypart_3;
  daypart_3.dow = current_dow;
  daypart_3.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_3.end_minute = current_time + base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_3);

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_.ShouldExclude(creative_ad));
}

TEST_F(BatAdsDaypartExclusionRuleTest, DisallowIfNoMatches) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;
  const std::string tomorrow_dow =
      base::NumberToString((exploded.day_of_week + 1) % 7);
  const std::string current_dow = base::NumberToString(exploded.day_of_week);

  CreativeDaypartInfo daypart_1;
  daypart_1.dow = tomorrow_dow;
  daypart_1.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_1.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_1);

  CreativeDaypartInfo daypart_2;
  daypart_2.dow = tomorrow_dow;
  daypart_2.start_minute = current_time + 2 * base::Time::kMinutesPerHour;
  daypart_2.end_minute = current_time + 3 * base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_2);

  CreativeDaypartInfo daypart_3;
  daypart_3.dow = current_dow;
  daypart_3.start_minute = current_time + base::Time::kMinutesPerHour;
  daypart_3.end_minute = current_time + 2 * base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_3);

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_.ShouldExclude(creative_ad));
}

TEST_F(BatAdsDaypartExclusionRuleTest, DisallowIfWrongDay) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;
  const std::string tomorrow_dow =
      base::NumberToString((exploded.day_of_week + 1) % 7);

  CreativeDaypartInfo daypart_1;
  daypart_1.dow = tomorrow_dow;
  daypart_1.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_1.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_1);

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_.ShouldExclude(creative_ad));
}

TEST_F(BatAdsDaypartExclusionRuleTest, DisallowIfWrongHours) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;
  const std::string current_dow = base::NumberToString(exploded.day_of_week);

  CreativeDaypartInfo daypart_1;
  daypart_1.dow = current_dow;
  daypart_1.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_1.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_1);

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_.ShouldExclude(creative_ad));
}

}  // namespace brave_ads
