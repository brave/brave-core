/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/prefs/pref_manager.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kPrefName[] = "FOOBAR";
}  // namespace

class BatAdsPrefManagerTest : public PrefManagerObserver, public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    PrefManager::GetInstance()->AddObserver(this);
  }

  void TearDown() override {
    PrefManager::GetInstance()->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnPrefDidChange(const std::string& /*path*/) override {
    pref_changed_ = true;
  }

  bool pref_changed_ = false;
};

TEST_F(BatAdsPrefManagerTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = PrefManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsPrefManagerTest, PrefChanged) {
  // Arrange

  // Act
  PrefManager::GetInstance()->OnPrefDidChange(kPrefName);

  // Assert
  EXPECT_TRUE(pref_changed_);
}

}  // namespace ads
