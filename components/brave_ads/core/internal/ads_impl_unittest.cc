/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/public/ads.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsImplTest : public test::TestBase {
 public:
  BraveAdsImplTest() : test::TestBase(/*is_integration_test=*/true) {}
};

TEST_F(BraveAdsImplTest, DoesNotInitializeWhenAlreadyInitialized) {
  // Act & Assert
  base::test::TestFuture<bool> test_future;
  GetAds().Initialize(/*mojom_wallet=*/nullptr, test_future.GetCallback());
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsImplTest, Shutdown) {
  // Act & Assert
  base::test::TestFuture<bool> test_future;
  GetAds().Shutdown(test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsImplTest, DoesNotShutdownWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> shutdown_future;
  GetAds().Shutdown(shutdown_future.GetCallback());
  ASSERT_TRUE(shutdown_future.Get());

  // Act & Assert
  base::test::TestFuture<bool> test_future;
  GetAds().Shutdown(test_future.GetCallback());
  EXPECT_FALSE(test_future.Get());
}

}  // namespace brave_ads
