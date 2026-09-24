/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

mojom::DBRowInfoPtr BuildCreativeAdRow(const std::string& creative_instance_id,
                                       const std::string& creative_set_id,
                                       int per_day,
                                       int per_week,
                                       int per_month,
                                       int total_max,
                                       double value,
                                       const std::string& condition_matchers,
                                       const std::string& target_url) {
  auto mojom_db_row = mojom::DBRowInfo::New();
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(creative_instance_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(creative_set_id));
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
      mojom::DBColumnValueUnion::NewStringValue(target_url));
  return mojom_db_row;
}

}  // namespace

TEST(BraveAdsCreativeAdsDatabaseTableUtilTest, MapAllFieldsFromMojomRow) {
  // Arrange
  mojom::DBRowInfoPtr mojom_db_row =
      BuildCreativeAdRow("creative-instance-id-foo", "creative-set-id-foo",
                         /*per_day=*/1, /*per_week=*/7, /*per_month=*/42,
                         /*total_max=*/100, /*value=*/0.05,
                         /*condition_matchers=*/"", "https://example.com");

  // Act
  const CreativeAdInfo creative_ad = CreativeAdFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_EQ("creative-instance-id-foo", creative_ad.creative_instance_id);
  EXPECT_EQ("creative-set-id-foo", creative_ad.creative_set_id);
  EXPECT_EQ(1, creative_ad.per_day);
  EXPECT_EQ(7, creative_ad.per_week);
  EXPECT_EQ(42, creative_ad.per_month);
  EXPECT_EQ(100, creative_ad.total_max);
  EXPECT_DOUBLE_EQ(0.05, creative_ad.value);
  EXPECT_EQ(GURL("https://example.com"), creative_ad.target_url);
}

TEST(BraveAdsCreativeAdsDatabaseTableUtilTest,
     MapEmptyConditionMatchersAsEmptyMap) {
  // Arrange
  mojom::DBRowInfoPtr mojom_db_row = BuildCreativeAdRow(
      "foo", "bar", 1, 7, 42, 100, 0.01, "", "https://foo.com");

  // Act
  const CreativeAdInfo creative_ad = CreativeAdFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_TRUE(creative_ad.condition_matchers.empty());
}

}  // namespace brave_ads::database::table
