/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table_util.h"

#include <optional>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

mojom::DBRowInfoPtr BuildDepositRow(const std::string& creative_instance_id,
                                    double value,
                                    base::Time expire_at) {
  auto mojom_db_row = mojom::DBRowInfo::New();
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewStringValue(creative_instance_id));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewDoubleValue(value));
  mojom_db_row->column_values_union.push_back(
      mojom::DBColumnValueUnion::NewTimeValue(expire_at));
  return mojom_db_row;
}

}  // namespace

class BraveAdsDepositsDatabaseTableUtilTest : public ::testing::Test {};

TEST_F(BraveAdsDepositsDatabaseTableUtilTest, MapAllFieldsFromMojomRow) {
  // Arrange
  const base::Time expire_at =
      base::Time::FromSecondsSinceUnixEpoch(1'000'000.0);
  mojom::DBRowInfoPtr mojom_db_row =
      BuildDepositRow("creative-instance-id", 0.01, expire_at);

  // Act
  const DepositInfo deposit = DepositFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_THAT(deposit,
              ::testing::FieldsAre("creative-instance-id", 0.01, expire_at));
}

TEST_F(BraveAdsDepositsDatabaseTableUtilTest, NoExpireAtWhenNull) {
  // Arrange
  mojom::DBRowInfoPtr mojom_db_row =
      BuildDepositRow("creative-instance-id", 0.0, /*expire_at=*/base::Time());

  // Act
  const DepositInfo deposit = DepositFromMojomRow(mojom_db_row);

  // Assert
  EXPECT_THAT(deposit, ::testing::FieldsAre("creative-instance-id", 0.0,
                                            /*expire_at=*/std::nullopt));
}

}  // namespace brave_ads::database::table
