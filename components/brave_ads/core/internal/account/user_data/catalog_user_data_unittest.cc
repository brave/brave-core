/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/catalog_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::user_data {

class BatAdsCatalogUserDataTest : public UnitTestBase {};

TEST_F(BatAdsCatalogUserDataTest, GetCatalog) {
  // Arrange
  SetCatalogId(kCatalogId);

  // Act
  const base::Value::Dict user_data = GetCatalog();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}]})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace brave_ads::user_data
