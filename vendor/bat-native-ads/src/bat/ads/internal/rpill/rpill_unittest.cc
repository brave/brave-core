/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/rpill/rpill.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsRPillTest : public UnitTestBase {
 protected:
  BatAdsRPillTest() = default;

  ~BatAdsRPillTest() override = default;
};

TEST_F(BatAdsRPillTest,
    IsUncertainFutureForAmazon) {
  // Arrange
  SysInfo sys_info;
  sys_info.manufacturer = "Amazon";
  sys_info.model = "Virtual Platform";
  SetSysInfo(sys_info);

  // Act
  const bool is_uncertain_future = IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillTest,
    IsUncertainFutureForVirtualBox) {
  // Arrange
  SysInfo sys_info;
  sys_info.manufacturer = "VirtualBox";
  sys_info.model = "innotek GmbH";
  SetSysInfo(sys_info);

  // Act
  const bool is_uncertain_future = IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillTest,
    IsUncertainFutureForVMWare) {
  // Arrange
  SysInfo sys_info;
  sys_info.manufacturer = "VMWare";
  sys_info.model = "Virtual Platform";
  SetSysInfo(sys_info);

  // Act
  const bool is_uncertain_future = IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillTest,
    IsUncertainFutureForXen) {
  // Arrange
  SysInfo sys_info;
  sys_info.manufacturer = "Xen";
  sys_info.model = "HVM domU";
  SetSysInfo(sys_info);

  // Act
  const bool is_uncertain_future = IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillTest,
    IsCertainFuture) {
  // Arrange
  SysInfo sys_info;
  sys_info.manufacturer = "SAMSUNG ELECTRONICS CO., LTD.";
  sys_info.model = "900X3N";
  SetSysInfo(sys_info);

  // Act
  const bool is_uncertain_future = IsUncertainFuture();

  // Assert
  EXPECT_FALSE(is_uncertain_future);
}

TEST_F(BatAdsRPillTest,
    IsCertainFutureForMissingSysInfo) {
  // Arrange
  SysInfo sys_info;
  SetSysInfo(sys_info);

  // Act
  const bool is_uncertain_future = IsUncertainFuture();

  // Assert
  EXPECT_FALSE(is_uncertain_future);
}

}  // namespace ads
