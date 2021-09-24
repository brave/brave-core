/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/daypart_frequency_cap.h"

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsDaypartFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsDaypartFrequencyCapTest() = default;

  ~BatAdsDaypartFrequencyCapTest() override = default;
};

TEST_F(BatAdsDaypartFrequencyCapTest, AllowIfDaypartsIsEmpty) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  // Act
  DaypartFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDaypartFrequencyCapTest, AllowIfRightDayAndHours) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = base::NumberToString(exploded.day_of_week);
  daypart_info.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time + base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info);

  // Act
  DaypartFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDaypartFrequencyCapTest, AllowForMultipleDays) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;

  CreativeDaypartInfo daypart_info;
  daypart_info.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time + base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info);

  // Act
  DaypartFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDaypartFrequencyCapTest, AllowIfOneMatchExists) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;
  std::string tomorrow_dow =
      base::NumberToString((exploded.day_of_week + 1) % 7);
  std::string current_dow = base::NumberToString(exploded.day_of_week);

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = tomorrow_dow;
  daypart_info.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info);

  CreativeDaypartInfo daypart_info_2;
  daypart_info_2.dow = tomorrow_dow;
  daypart_info_2.start_minute = current_time + 2 * base::Time::kMinutesPerHour;
  daypart_info_2.end_minute = current_time + 3 * base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info_2);

  CreativeDaypartInfo daypart_info_3;
  daypart_info_3.dow = current_dow;
  daypart_info_3.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info_3.end_minute = current_time + base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info_3);

  // Act
  DaypartFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDaypartFrequencyCapTest, DisallowIfNoMatches) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;
  std::string tomorrow_dow =
      base::NumberToString((exploded.day_of_week + 1) % 7);
  std::string current_dow = base::NumberToString(exploded.day_of_week);

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = tomorrow_dow;
  daypart_info.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info);

  CreativeDaypartInfo daypart_info_2;
  daypart_info_2.dow = tomorrow_dow;
  daypart_info_2.start_minute = current_time + 2 * base::Time::kMinutesPerHour;
  daypart_info_2.end_minute = current_time + 3 * base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info_2);

  CreativeDaypartInfo daypart_info_3;
  daypart_info_3.dow = current_dow;
  daypart_info_3.start_minute = current_time + base::Time::kMinutesPerHour;
  daypart_info_3.end_minute = current_time + 2 * base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info_3);

  // Act
  DaypartFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsDaypartFrequencyCapTest, DisallowIfWrongDay) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;

  // Go to next day
  std::string tomorrow_dow =
      base::NumberToString((exploded.day_of_week + 1) % 7);

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = tomorrow_dow;
  daypart_info.start_minute = current_time - 2 * base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info);

  // Act
  DaypartFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsDaypartFrequencyCapTest, DisallowIfWrongHours) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;

  base::Time::Exploded exploded;
  Now().LocalExplode(&exploded);
  const int current_time =
      base::Time::kMinutesPerHour * exploded.hour + exploded.minute;
  std::string current_dow = base::NumberToString(exploded.day_of_week);

  CreativeDaypartInfo daypart_info;
  daypart_info.dow = current_dow;
  daypart_info.start_minute = current_time - base::Time::kMinutesPerHour;
  daypart_info.end_minute = current_time - base::Time::kMinutesPerHour;
  ad.dayparts.push_back(daypart_info);

  // Act
  DaypartFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
