/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_util.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsLegacyClientMigrationTest : public test::TestBase {
 protected:
  void SetUpMocks() override {
    test::SetProfileBooleanPrefValue(prefs::kHasMigratedClientState, false);
  }
};

TEST_F(BraveAdsLegacyClientMigrationTest, Migrate) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(kClientJsonFilename));

  // Act & Assert
  base::MockCallback<InitializeCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  MigrateClientState(callback.Get());

  EXPECT_TRUE(HasMigratedClientState());
}

TEST_F(BraveAdsLegacyClientMigrationTest, ResetMalformedState) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      test::kMalformedJsonFilename, kClientJsonFilename));

  // Act & Assert
  base::MockCallback<InitializeCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  MigrateClientState(callback.Get());

  EXPECT_TRUE(HasMigratedClientState());
}

}  // namespace brave_ads
