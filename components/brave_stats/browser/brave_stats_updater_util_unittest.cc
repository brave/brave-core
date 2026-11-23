/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_stats {

namespace {

base::Time TimeFromLocal(int year, int month, int day) {
  base::Time::Exploded exploded = {
      .year = year, .month = month, .day_of_month = day};
  base::Time time;
  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  return time;
}

base::Time TimeFromUTC(int year, int month, int day) {
  base::Time::Exploded exploded = {
      .year = year, .month = month, .day_of_month = day};
  base::Time time;
  EXPECT_TRUE(base::Time::FromUTCExploded(exploded, &time));
  return time;
}

}  // namespace

class BraveStatsUpdaterUtilTest : public testing::Test {};

TEST_F(BraveStatsUpdaterUtilTest, GetYMDAsDateParsesValidLocalDate) {
  std::optional<base::Time> time =
      GetYMDAsDate("2018-06-22", /*use_utc=*/false);
  ASSERT_TRUE(time);
  base::Time::Exploded exploded;
  time->LocalExplode(&exploded);
  EXPECT_THAT(exploded, testing::FieldsAre(/*year=*/2018, /*month=*/6,
                                           /*day_of_week=*/5,
                                           /*day_of_month=*/22, /*hour=*/0,
                                           /*minute=*/0, /*second=*/0,
                                           /*millisecond=*/0));
}

TEST_F(BraveStatsUpdaterUtilTest, GetYMDAsDateParsesValidUtcDate) {
  std::optional<base::Time> time = GetYMDAsDate("2018-06-22", /*use_utc=*/true);
  ASSERT_TRUE(time);
  base::Time::Exploded exploded;
  time->UTCExplode(&exploded);
  EXPECT_THAT(exploded, testing::FieldsAre(/*year=*/2018, /*month=*/6,
                                           /*day_of_week=*/5,
                                           /*day_of_month=*/22, /*hour=*/0,
                                           /*minute=*/0, /*second=*/0,
                                           /*millisecond=*/0));
}

TEST_F(BraveStatsUpdaterUtilTest, GetYMDAsDateReturnsNulloptForEmptyLocalDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"", /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest, GetYMDAsDateReturnsNulloptForEmptyUtcDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"", /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForInvalidOnePieceLocalDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018", /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForInvalidOnePieceUtcDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018", /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForInvalidTwoPiecesLocalDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018-06", /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForInvalidTwoPiecesUtcDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018-06", /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForNonNumericLocalDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"xxxx-xx-xx", /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForNonNumericUtcDate) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"xxxx-xx-xx", /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForInvalidLocalMonth) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018-13-01", /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForInvalidUtcMonth) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018-13-01", /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetYMDAsDateReturnsNulloptForInvalidLocalDay) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018-02-32", /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest, GetYMDAsDateReturnsNulloptForInvalidUtcDay) {
  EXPECT_FALSE(GetYMDAsDate(/*ymd=*/"2018-02-32", /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest, GetDateAsYMDFormatsLocalDate) {
  const base::Time time = TimeFromLocal(2018, 6, 22);
  EXPECT_EQ("2018-06-22", GetDateAsYMD(time, /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest, GetDateAsYMDCorrectlyFormatsUtcDate) {
  const base::Time time = TimeFromUTC(2018, 6, 22);
  EXPECT_EQ("2018-06-22", GetDateAsYMD(time, /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest,
       GetDateAsYMDReturnsOriginalStringForLocalTime) {
  std::optional<base::Time> time =
      GetYMDAsDate(/*ymd=*/"2018-06-22", /*use_utc=*/false);
  ASSERT_TRUE(time);
  EXPECT_EQ("2018-06-22", GetDateAsYMD(*time, /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest, GetDateAsYMDReturnsOriginalStringForUtcTime) {
  std::optional<base::Time> time =
      GetYMDAsDate(/*ymd=*/"2018-06-22", /*use_utc=*/true);
  ASSERT_TRUE(time);
  EXPECT_EQ("2018-06-22", GetDateAsYMD(*time, /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest, GetYMDAsDateForLocalTime) {
  const base::Time time = TimeFromLocal(/*year=*/2018, /*month=*/6, /*day=*/22);
  const std::string ymd = GetDateAsYMD(time, /*use_utc=*/false);
  EXPECT_EQ(time, GetYMDAsDate(ymd, /*use_utc=*/false));
}

TEST_F(BraveStatsUpdaterUtilTest, GetYMDAsDateForUtcTime) {
  const base::Time time = TimeFromUTC(/*year=*/2018, /*month=*/6, /*day=*/22);
  const std::string ymd = GetDateAsYMD(time, /*use_utc=*/true);
  EXPECT_EQ(time, GetYMDAsDate(ymd, /*use_utc=*/true));
}

TEST_F(BraveStatsUpdaterUtilTest, GetIsoWeekNumber) {
  base::Time::Exploded exploded;
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  exploded.day_of_week = 1;
  exploded.day_of_month = 29;
  exploded.month = 7;
  exploded.year = 2019;

  base::Time time;
  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(GetIsoWeekNumber(time), 31);

  exploded.day_of_month = 30;
  exploded.month = 9;

  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(GetIsoWeekNumber(time), 40);

  exploded.day_of_month = 1;
  exploded.month = 9;
  exploded.day_of_week = 0;

  EXPECT_TRUE(base::Time::FromLocalExploded(exploded, &time));
  EXPECT_EQ(GetIsoWeekNumber(time), 35);
}

TEST_F(BraveStatsUpdaterUtilTest, UsageBitstringDaily) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-03-30", &last_reported_use));

  EXPECT_EQ(0b001, UsageBitfieldFromTimestamp(last_use, last_reported_use));
}

TEST_F(BraveStatsUpdaterUtilTest, UsageBitstringWeekly) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-03-26", &last_reported_use));

  EXPECT_EQ(0b011, UsageBitfieldFromTimestamp(last_use, last_reported_use));
}

TEST_F(BraveStatsUpdaterUtilTest, UsageBitstringMonthlySameWeek) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-07-01", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-06-30", &last_reported_use));

  EXPECT_EQ(0b101, UsageBitfieldFromTimestamp(last_use, last_reported_use));
}

TEST_F(BraveStatsUpdaterUtilTest, UsageBitstringMonthlyDiffWeek) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-01", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-02-15", &last_reported_use));

  EXPECT_EQ(0b111, UsageBitfieldFromTimestamp(last_use, last_reported_use));
}

TEST_F(BraveStatsUpdaterUtilTest, UsageBitstringInactive) {
  base::Time last_reported_use;
  base::Time last_use;

  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_use));
  EXPECT_TRUE(base::Time::FromString("2020-03-31", &last_reported_use));

  EXPECT_EQ(0b000, UsageBitfieldFromTimestamp(last_use, last_reported_use));
}

TEST_F(BraveStatsUpdaterUtilTest, UsageBitstringNeverUsed) {
  EXPECT_EQ(0b000, UsageBitfieldFromTimestamp(base::Time(), base::Time()));
}

}  // namespace brave_stats
