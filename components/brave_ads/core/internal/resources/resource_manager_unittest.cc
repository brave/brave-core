/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/resource_manager.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {
constexpr char kResourceId[] = "bejenkminijgplakmkmcgkhjjnkelbld";
}  // namespace

class BatAdsResourceManagerTest : public ResourceManagerObserver,
                                  public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    ResourceManager::GetInstance()->AddObserver(this);
  }

  void TearDown() override {
    ResourceManager::GetInstance()->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnResourceDidUpdate(const std::string& /*id*/) override {
    resource_did_update_ = true;
  }

  bool resource_did_update_ = false;
};

TEST_F(BatAdsResourceManagerTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = ResourceManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsResourceManagerTest, UpdateResource) {
  // Arrange

  // Act
  ResourceManager::GetInstance()->UpdateResource(kResourceId);

  // Assert
  EXPECT_TRUE(resource_did_update_);
}

}  // namespace brave_ads
