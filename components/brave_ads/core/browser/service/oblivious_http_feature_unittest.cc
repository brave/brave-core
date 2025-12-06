/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/oblivious_http_feature.h"

#include "base/feature_list.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsObliviousHttpFeatureTest, IsDisabled) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(kAdsObliviousHttpFeature));
}

TEST(BraveAdsObliviousHttpFeatureTest, ShouldNotSupport) {
  EXPECT_FALSE(kShouldSupportOhttp.Get());
}

TEST(BraveAdsObliviousHttpFeatureTest, TimeoutDuration) {
  EXPECT_EQ(base::Seconds(3), kOhttpTimeoutDuration.Get());
}

TEST(BraveAdsObliviousHttpFeatureTest, KeyConfigExpiresAfter) {
  EXPECT_EQ(base::Days(3), kOhttpKeyConfigExpiresAfter.Get());
}

TEST(BraveAdsObliviousHttpFeatureTest, InitialKeyConfigBackoffDelay) {
  EXPECT_EQ(base::Minutes(5), kOhttpKeyConfigInitialBackoffDelay.Get());
}

TEST(BraveAdsObliviousHttpFeatureTest, MaxKeyConfigBackoffDelay) {
  EXPECT_EQ(base::Days(1), kOhttpKeyConfigMaxBackoffDelay.Get());
}

}  // namespace brave_ads
