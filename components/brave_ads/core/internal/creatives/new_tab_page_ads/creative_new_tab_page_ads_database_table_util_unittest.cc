/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

// Column layout (indices 0-24):
//  0: creative_instance_id    1: creative_set_id     2: campaign_id
//  3: metric_type             4: start_at            5: end_at
//  6: daily_cap               7: advertiser_id       8: priority
//  9: per_day                10: per_week           11: per_month
// 12: total_max              13: value              14: condition_matchers
// 15: segment                16: geo_target         17: target_url
// 18: wallpaper_type         19: company_name       20: alt
// 21: pass_through_rate
// 22: daypart.days_of_week   23: daypart.start_minute
// 24: daypart.end_minute
mojom::DBRowInfoPtr BuildCreativeNewTabPageAdRow(
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
    const std::string& condition_matchers,
    const std::string& segment,
    const std::string& geo_target,
    const std::string& target_url,
    const std::string& wallpaper_type,
    const std::string& company_name,
    const std::string& alt,
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
      mojom::DBColumnValueUnion::NewStringValue(condition_matchers));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(segment));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(geo_target));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(target_url));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(wallpaper_type));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(company_name));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(alt));
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

class BraveAdsCreativeNewTabPageAdsDatabaseTableUtilTest
    : public ::testing::Test {};

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableUtilTest,
       MapAllFieldsFromMojomRow) {
  // Arrange
  const base::Time start_at =
      base::Time::FromSecondsSinceUnixEpoch(1'000'000.0);
  const base::Time end_at = base::Time::FromSecondsSinceUnixEpoch(2'000'000.0);
  mojom::DBRowInfoPtr mojom_db_row = BuildCreativeNewTabPageAdRow(
      "creative-instance-id-foo", "creative-set-id-foo", "campaign-id-foo",
      "confirmation", start_at, end_at, /*daily_cap=*/20, "advertiser-id-foo",
      /*priority=*/10, /*per_day=*/1, /*per_week=*/7, /*per_month=*/42,
      /*total_max=*/100, /*value=*/0.05, /*condition_matchers=*/"",
      "segment-foo", "US", "https://example.com", "image", "Brave Company",
      "Alt text foo.", /*pass_through_rate=*/1.0, "0123456",
      /*start_minute=*/0, /*end_minute=*/1439);

  // Act
  const CreativeNewTabPageAdInfo creative_ad =
      CreativeNewTabPageAdFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_EQ("creative-instance-id-foo", creative_ad.creative_instance_id);
  EXPECT_EQ("creative-set-id-foo", creative_ad.creative_set_id);
  EXPECT_EQ("campaign-id-foo", creative_ad.campaign_id);
  EXPECT_EQ(mojom::NewTabPageAdMetricType::kConfirmation,
            creative_ad.metric_type);
  EXPECT_EQ(start_at, creative_ad.start_at);
  EXPECT_EQ(end_at, creative_ad.end_at);
  EXPECT_EQ(20, creative_ad.daily_cap);
  EXPECT_EQ("advertiser-id-foo", creative_ad.advertiser_id);
  EXPECT_EQ(10, creative_ad.priority);
  EXPECT_EQ(1, creative_ad.per_day);
  EXPECT_EQ(7, creative_ad.per_week);
  EXPECT_EQ(42, creative_ad.per_month);
  EXPECT_EQ(100, creative_ad.total_max);
  EXPECT_DOUBLE_EQ(0.05, creative_ad.value);
  EXPECT_EQ("segment-foo", creative_ad.segment);
  EXPECT_TRUE(creative_ad.geo_targets.contains("US"));
  EXPECT_EQ(GURL("https://example.com"), creative_ad.target_url);
  EXPECT_EQ(CreativeNewTabPageAdWallpaperType::kImage,
            creative_ad.wallpaper_type);
  EXPECT_EQ("Brave Company", creative_ad.company_name);
  EXPECT_EQ("Alt text foo.", creative_ad.alt);
  EXPECT_DOUBLE_EQ(1.0, creative_ad.pass_through_rate);

  ASSERT_EQ(1U, creative_ad.dayparts.size());
  const CreativeDaypartInfo& daypart = *creative_ad.dayparts.begin();
  EXPECT_EQ("0123456", daypart.days_of_week);
  EXPECT_EQ(0, daypart.start_minute);
  EXPECT_EQ(1439, daypart.end_minute);
}

TEST_F(BraveAdsCreativeNewTabPageAdsDatabaseTableUtilTest,
       MapRichMediaWallpaperType) {
  // Arrange
  const base::Time start_at =
      base::Time::FromSecondsSinceUnixEpoch(1'000'000.0);
  const base::Time end_at = base::Time::FromSecondsSinceUnixEpoch(2'000'000.0);
  mojom::DBRowInfoPtr mojom_db_row = BuildCreativeNewTabPageAdRow(
      "foo", "bar", "baz", "disabled", start_at, end_at, 1, "qux", 1, 1, 7, 42,
      100, 0.01, "", "untargeted", "US", "https://foo.com", "richMedia",
      "company", "alt", 0.5, "0123456", 0, 1439);

  // Act
  const CreativeNewTabPageAdInfo creative_ad =
      CreativeNewTabPageAdFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_EQ(CreativeNewTabPageAdWallpaperType::kRichMedia,
            creative_ad.wallpaper_type);
}

}  // namespace brave_ads::database::table
