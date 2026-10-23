/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/services/bat_ads_service_factory_impl.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_ads/services/bat_ads_service_feature.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsServiceFactoryImplTest : public testing::Test {
 public:
  BatAdsServiceFactoryImplTest() {
    // Pinned to a non-zero value so a `Launch` superseded by a second
    // `Launch` or an `Invalidate` before its delayed bind runs can be
    // observed dropping its receiver, rather than the bind having already
    // raced to completion with a zero delay.
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kInProcessBraveAdsService, {{"startup_delay", "100ms"}});
  }

 protected:
  content::BrowserTaskEnvironment task_environment_{
      content::BrowserTaskEnvironment::TimeSource::MOCK_TIME};

  base::test::ScopedFeatureList scoped_feature_list_;

  std::unique_ptr<BatAdsServiceFactoryImpl> factory_ =
      std::make_unique<BatAdsServiceFactoryImpl>();
};

TEST_F(BatAdsServiceFactoryImplTest, LaunchBindsServiceWhenNotSuperseded) {
  // Arrange
  mojo::Remote<bat_ads::mojom::BatAdsService> remote = factory_->Launch();

  // Act
  task_environment_.FastForwardBy(base::Seconds(1));
  remote.FlushForTesting();

  // Assert
  EXPECT_TRUE(remote.is_connected());
}

TEST_F(BatAdsServiceFactoryImplTest, LaunchDisconnectsPreviousRemote) {
  // Arrange
  mojo::Remote<bat_ads::mojom::BatAdsService> first_remote = factory_->Launch();

  // Act: supersede the first launch before its delayed bind runs.
  mojo::Remote<bat_ads::mojom::BatAdsService> second_remote =
      factory_->Launch();
  task_environment_.FastForwardBy(base::Seconds(1));
  first_remote.FlushForTesting();
  second_remote.FlushForTesting();

  // Assert
  EXPECT_FALSE(first_remote.is_connected());
  EXPECT_TRUE(second_remote.is_connected());
}

TEST_F(BatAdsServiceFactoryImplTest, InvalidatingDisconnectsUnboundRemote) {
  // Arrange
  mojo::Remote<bat_ads::mojom::BatAdsService> remote = factory_->Launch();

  // Act
  factory_->Invalidate();
  task_environment_.FastForwardBy(base::Seconds(1));
  remote.FlushForTesting();

  // Assert
  EXPECT_FALSE(remote.is_connected());
}

TEST_F(BatAdsServiceFactoryImplTest, LaunchBindsServiceAfterPriorInvalidate) {
  // Arrange: disconnect, then restart before the disconnected launch's
  // delayed bind would have run.
  mojo::Remote<bat_ads::mojom::BatAdsService> invalidated_remote =
      factory_->Launch();
  factory_->Invalidate();

  // Act
  mojo::Remote<bat_ads::mojom::BatAdsService> remote = factory_->Launch();
  task_environment_.FastForwardBy(base::Seconds(1));
  invalidated_remote.FlushForTesting();
  remote.FlushForTesting();

  // Assert
  EXPECT_FALSE(invalidated_remote.is_connected());
  EXPECT_TRUE(remote.is_connected());
}

TEST_F(BatAdsServiceFactoryImplTest, InvalidatingDisconnectsBoundRemote) {
  // Arrange: let the delayed bind complete and connect first.
  mojo::Remote<bat_ads::mojom::BatAdsService> remote = factory_->Launch();
  task_environment_.FastForwardBy(base::Seconds(1));
  remote.FlushForTesting();
  ASSERT_TRUE(remote.is_connected());

  // Act
  factory_->Invalidate();
  remote.FlushForTesting();

  // Assert
  EXPECT_FALSE(remote.is_connected());
}

TEST_F(BatAdsServiceFactoryImplTest,
       DestroyingFactoryDoesNotCrashPendingLaunch) {
  // Arrange
  mojo::Remote<bat_ads::mojom::BatAdsService> remote = factory_->Launch();

  // Act: destroy the factory while its delayed bind is still pending.
  factory_.reset();
  task_environment_.FastForwardBy(base::Seconds(1));
  remote.FlushForTesting();

  // Assert
  EXPECT_TRUE(remote.is_connected());
}

}  // namespace brave_ads
