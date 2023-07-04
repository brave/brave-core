/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/build_channel_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBuildChannelUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsBuildChannelUserDataTest, BuildBuildChannelUserData) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(R"({"buildChannel":"release"})"),
            BuildBuildChannelUserData());
}

}  // namespace brave_ads
