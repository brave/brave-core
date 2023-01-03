/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/locale/locale_manager.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kLocale[] = "en_GB";
}  // namespace

class BatAdsLocaleManagerTest : public LocaleManagerObserver,
                                public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    LocaleManager::GetInstance()->AddObserver(this);
  }

  void TearDown() override {
    LocaleManager::GetInstance()->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnLocaleDidChange(const std::string& /*locale*/) override {
    locale_did_change_ = true;
  }

  bool locale_did_change_ = false;
};

TEST_F(BatAdsLocaleManagerTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = LocaleManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsLocaleManagerTest, LocaleDidChange) {
  // Arrange

  // Act
  LocaleManager::GetInstance()->OnLocaleDidChange(kLocale);

  // Assert
  EXPECT_TRUE(locale_did_change_);
}

TEST_F(BatAdsLocaleManagerTest, LocaleDidNotChange) {
  // Arrange

  // Act
  LocaleManager::GetInstance()->OnLocaleDidChange(kDefaultLocale);

  // Assert
  EXPECT_FALSE(locale_did_change_);
}

}  // namespace ads
