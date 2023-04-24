/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/catalog_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCatalogUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsCatalogUserDataTest, BuildCatalogUserData) {
  // Arrange
  SetCatalogId(kCatalogId);

  // Act

  // Assert
  EXPECT_EQ(
      base::test::ParseJsonDict(
          R"({"catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}]})"),
      BuildCatalogUserData());
}

}  // namespace brave_ads
