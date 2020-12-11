/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/rpill/rpill_helper.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/sys_info_helper_mock.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsRPillHelperTest : public UnitTestBase {
 protected:
  BatAdsRPillHelperTest() = default;

  ~BatAdsRPillHelperTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    RPillHelper::GetInstance()->set_for_testing(nullptr);
  }
};

TEST_F(BatAdsRPillHelperTest,
    IsUncertainFutureForAmazon) {
  // Arrange
  base::SysInfo::HardwareInfo hardware;
  hardware.manufacturer = "Amazon";
  hardware.model = "Virtual Platform";
  MockSysInfoHelper(sys_info_helper_mock_, hardware);

  // Act
  const bool is_uncertain_future =
      RPillHelper::GetInstance()->IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillHelperTest,
    IsUncertainFutureForVirtualBox) {
  // Arrange
  base::SysInfo::HardwareInfo hardware;
  hardware.manufacturer = "VirtualBox";
  hardware.model = "innotek GmbH";
  MockSysInfoHelper(sys_info_helper_mock_, hardware);

  // Act
  const bool is_uncertain_future =
      RPillHelper::GetInstance()->IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillHelperTest,
    IsUncertainFutureForVMWare) {
  // Arrange
  base::SysInfo::HardwareInfo hardware;
  hardware.manufacturer = "VMWare";
  hardware.model = "Virtual Platform";
  MockSysInfoHelper(sys_info_helper_mock_, hardware);

  // Act
  const bool is_uncertain_future =
      RPillHelper::GetInstance()->IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillHelperTest,
    IsUncertainFutureForXen) {
  // Arrange
  base::SysInfo::HardwareInfo hardware;
  hardware.manufacturer = "Xen";
  hardware.model = "HVM domU";
  MockSysInfoHelper(sys_info_helper_mock_, hardware);

  // Act
  const bool is_uncertain_future =
      RPillHelper::GetInstance()->IsUncertainFuture();

  // Assert
  EXPECT_TRUE(is_uncertain_future);
}

TEST_F(BatAdsRPillHelperTest,
    IsCertainFuture) {
  // Arrange
  base::SysInfo::HardwareInfo hardware;
  hardware.manufacturer = "SAMSUNG ELECTRONICS CO., LTD.";
  hardware.model = "900X3N";
  MockSysInfoHelper(sys_info_helper_mock_, hardware);

  // Act
  const bool is_uncertain_future =
      RPillHelper::GetInstance()->IsUncertainFuture();

  // Assert
  EXPECT_FALSE(is_uncertain_future);
}

TEST_F(BatAdsRPillHelperTest,
    IsCertainFutureForMissingSysInfo) {
  // Arrange
  base::SysInfo::HardwareInfo hardware;
  MockSysInfoHelper(sys_info_helper_mock_, hardware);

  // Act
  const bool is_uncertain_future =
      RPillHelper::GetInstance()->IsUncertainFuture();

  // Assert
  EXPECT_FALSE(is_uncertain_future);
}

}  // namespace ads
