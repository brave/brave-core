/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/headers/via_header_util.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/sys_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsViaHeaderUtilTest : public UnitTestBase {};

TEST_F(BatAdsViaHeaderUtilTest, BuildViaHeaderForUncertainFuture) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  // Act
  const std::string via_header = server::BuildViaHeader();

  // Assert
  const std::string expect_via_header =
      "Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1)";

  EXPECT_EQ(expect_via_header, via_header);
}

TEST_F(BatAdsViaHeaderUtilTest, BuildViaHeaderForABrightFuture) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  // Act
  const std::string via_header = server::BuildViaHeader();

  // Assert
  const std::string expect_via_header =
      "Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1)";

  EXPECT_EQ(expect_via_header, via_header);
}

}  // namespace ads
