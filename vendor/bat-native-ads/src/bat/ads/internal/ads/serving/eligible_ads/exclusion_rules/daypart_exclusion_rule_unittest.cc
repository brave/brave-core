/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/daypart_exclusion_rule.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsDaypartExclusionRuleTest : public UnitTestBase {};

TEST_F(BatAdsDaypartExclusionRuleTest, AllowIfDaypartsIsEmpty) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act
  DaypartExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDaypartExclusionRuleTest, AllowIfRightDayAndHours) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = base::NumberToString(exploded.day_of_week);
  daypart_info.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time + base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info);

  // Act
  DaypartExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDaypartExclusionRuleTest, AllowForMultipleDays) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;

  CreativeDaypartInfo daypart_info;
  daypart_info.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time + base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info);

  // Act
  DaypartExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
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

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = tomorrow_dow;
  daypart_info.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info);

  CreativeDaypartInfo daypart_info_2;
  daypart_info_2.dow = tomorrow_dow;
  daypart_info_2.start_minute = current_time + 2 * base::Time::kMinutesPerHour;
  daypart_info_2.end_minute = current_time + 3 * base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info_2);

  CreativeDaypartInfo daypart_info_3;
  daypart_info_3.dow = current_dow;
  daypart_info_3.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info_3.end_minute = current_time + base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info_3);

  // Act
  DaypartExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
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

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = tomorrow_dow;
  daypart_info.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info);

  CreativeDaypartInfo daypart_info_2;
  daypart_info_2.dow = tomorrow_dow;
  daypart_info_2.start_minute = current_time + 2 * base::Time::kMinutesPerHour;
  daypart_info_2.end_minute = current_time + 3 * base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info_2);

  CreativeDaypartInfo daypart_info_3;
  daypart_info_3.dow = current_dow;
  daypart_info_3.start_minute = current_time + base::Time::kMinutesPerHour;
  daypart_info_3.end_minute = current_time + 2 * base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info_3);

  // Act
  DaypartExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
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

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = tomorrow_dow;
  daypart_info.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info);

  // Act
  DaypartExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
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

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = current_dow;
  daypart_info.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  creative_ad.dayparts.push_back(daypart_info);

  // Act
  DaypartExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
