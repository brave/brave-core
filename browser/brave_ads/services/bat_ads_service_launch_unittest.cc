/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/services/bat_ads_service_launch.h"

#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsServiceLaunchTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;

  scoped_refptr<BatAdsServiceLaunch> launch_ =
      base::MakeRefCounted<BatAdsServiceLaunch>();
};

TEST_F(BatAdsServiceLaunchTest, BindConnectsReceiver) {
  // Arrange
  mojo::Remote<bat_ads::mojom::BatAdsService> remote;

  // Act
  launch_->Bind(remote.BindNewPipeAndPassReceiver());
  remote.FlushForTesting();

  // Assert
  EXPECT_TRUE(remote.is_connected());
}

TEST_F(BatAdsServiceLaunchTest, BindAfterCancelDoesNotConnectReceiver) {
  // Arrange
  mojo::Remote<bat_ads::mojom::BatAdsService> remote;
  launch_->Cancel();

  // Act
  launch_->Bind(remote.BindNewPipeAndPassReceiver());
  remote.FlushForTesting();

  // Assert
  EXPECT_FALSE(remote.is_connected());
}

TEST_F(BatAdsServiceLaunchTest, CancelAfterBindDisconnectsReceiver) {
  // Arrange
  mojo::Remote<bat_ads::mojom::BatAdsService> remote;
  launch_->Bind(remote.BindNewPipeAndPassReceiver());
  remote.FlushForTesting();
  ASSERT_TRUE(remote.is_connected());

  // Act
  launch_->Cancel();
  remote.FlushForTesting();

  // Assert
  EXPECT_FALSE(remote.is_connected());
}

TEST_F(BatAdsServiceLaunchTest, CancelWithoutBindDoesNotCrash) {
  // Act & Assert
  launch_->Cancel();
}

}  // namespace brave_ads
