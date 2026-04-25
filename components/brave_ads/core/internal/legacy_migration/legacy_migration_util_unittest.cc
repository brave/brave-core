/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/legacy_migration_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsLegacyMigrationUtilTest : public test::TestBase {};

TEST_F(BraveAdsLegacyMigrationUtilTest, DeleteFile) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, Remove("foo.json", ::testing::_ /*callback=*/));

  // Act & Assert
  MaybeDeleteFile("foo.json");
}

}  // namespace brave_ads
