/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_credential_json_writer.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRewardCredentialJsonWriterTest : public UnitTestBase {};

TEST_F(BraveAdsRewardCredentialJsonWriterTest, WriteRewardCredential) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(
      R"~({"signature":"PblFP7WI3ZC3aAX73A9UvBdqnvl87Wx8nnz9DIbhNjxbFamMZGbwn5Hi+FXsSXg2GZ671rCwQ6Xpwe61JjeW9Q==","t":"IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXxw=="})~",
      json::writer::WriteRewardCredential(
          BuildReward(),
          /*payload*/ "definition: the weight of a payload"));
}

}  // namespace brave_ads
