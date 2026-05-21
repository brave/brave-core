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

}  // namespace brave_stats
