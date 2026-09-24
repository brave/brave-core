/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

mojom::DBRowInfoPtr BuildCreativeNotificationAdRow(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const std::string& campaign_id,
    const std::string& metric_type,
    base::Time start_at,
    base::Time end_at,
    int daily_cap,
    const std::string& advertiser_id,
    int priority,
    int per_day,
    int per_week,
    int per_month,
    int total_max,
    double value,
    const std::string& segment,
    const std::string& geo_target,
    const std::string& target_url,
    const std::string& title,
    const std::string& body,
    double pass_through_rate,
    const std::string& days_of_week,
    int start_minute,
    int end_minute) {
  auto mojom_db_row = mojom::DBRowInfo::New();
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(creative_instance_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(creative_set_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(campaign_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(metric_type));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewTimeValue(start_at));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewTimeValue(end_at));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(daily_cap));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(advertiser_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(priority));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(per_day));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(per_week));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(per_month));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(total_max));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewDoubleValue(value));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(segment));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(geo_target));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(target_url));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(title));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(body));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewDoubleValue(pass_through_rate));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(days_of_week));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(start_minute));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewIntValue(end_minute));
  return mojom_db_row;
}

}  // namespace

class BraveAdsCreativeNotificationAdsDatabaseTableUtilTest
    : public ::testing::Test {};

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableUtilTest,
       MapAllFieldsFromMojomRow) {
  // Arrange
  const base::Time start_at =
      base::Time::FromSecondsSinceUnixEpoch(1'000'000.0);
  const base::Time end_at = base::Time::FromSecondsSinceUnixEpoch(2'000'000.0);
  mojom::DBRowInfoPtr mojom_db_row = BuildCreativeNotificationAdRow(
      "creative-instance-id-foo", "creative-set-id-foo", "campaign-id-foo",
      "disabled", start_at, end_at, /*daily_cap=*/10, "advertiser-id-foo",
      /*priority=*/5, /*per_day=*/1, /*per_week=*/7, /*per_month=*/42,
      /*total_max=*/100, /*value=*/0.05, "segment-foo", "US",
      "https://example.com", "Ad Title Foo", "Ad body foo.",
      /*pass_through_rate=*/0.5, "0123456", /*start_minute=*/0,
      /*end_minute=*/1439);

  // Act
  const CreativeNotificationAdInfo creative_ad =
      CreativeNotificationAdFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_EQ("creative-instance-id-foo", creative_ad.creative_instance_id);
  EXPECT_EQ("creative-set-id-foo", creative_ad.creative_set_id);
  EXPECT_EQ("campaign-id-foo", creative_ad.campaign_id);
  EXPECT_EQ(mojom::NewTabPageAdMetricType::kDisabled, creative_ad.metric_type);
  EXPECT_EQ(start_at, creative_ad.start_at);
  EXPECT_EQ(end_at, creative_ad.end_at);
  EXPECT_EQ(10, creative_ad.daily_cap);
  EXPECT_EQ("advertiser-id-foo", creative_ad.advertiser_id);
  EXPECT_EQ(5, creative_ad.priority);
  EXPECT_EQ(1, creative_ad.per_day);
  EXPECT_EQ(7, creative_ad.per_week);
  EXPECT_EQ(42, creative_ad.per_month);
  EXPECT_EQ(100, creative_ad.total_max);
  EXPECT_DOUBLE_EQ(0.05, creative_ad.value);
  EXPECT_EQ("segment-foo", creative_ad.segment);
  EXPECT_TRUE(creative_ad.geo_targets.contains("US"));
  EXPECT_EQ(GURL("https://example.com"), creative_ad.target_url);
  EXPECT_EQ("Ad Title Foo", creative_ad.title);
  EXPECT_EQ("Ad body foo.", creative_ad.body);
  EXPECT_DOUBLE_EQ(0.5, creative_ad.pass_through_rate);

  ASSERT_EQ(1U, creative_ad.dayparts.size());
  const CreativeDaypartInfo& daypart = *creative_ad.dayparts.begin();
  EXPECT_EQ("0123456", daypart.days_of_week);
  EXPECT_EQ(0, daypart.start_minute);
  EXPECT_EQ(1439, daypart.end_minute);
}

TEST_F(BraveAdsCreativeNotificationAdsDatabaseTableUtilTest,
       MapUnknownMetricTypeAsUndefined) {
  // Arrange
  const base::Time start_at =
      base::Time::FromSecondsSinceUnixEpoch(1'000'000.0);
  const base::Time end_at = base::Time::FromSecondsSinceUnixEpoch(2'000'000.0);
  mojom::DBRowInfoPtr mojom_db_row = BuildCreativeNotificationAdRow(
      "foo", "bar", "baz", /*metric_type=*/"", start_at, end_at, 1, "qux", 1, 1,
      7, 42, 100, 0.01, "untargeted", "US", "https://foo.com", "title", "body",
      1.0, "0123456", 0, 1439);

  // Act
  const CreativeNotificationAdInfo creative_ad =
      CreativeNotificationAdFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_EQ(mojom::NewTabPageAdMetricType::kUndefined, creative_ad.metric_type);
}

}  // namespace brave_ads::database::table
