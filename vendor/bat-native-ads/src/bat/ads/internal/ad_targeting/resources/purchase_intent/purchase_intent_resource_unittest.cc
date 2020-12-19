/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/resources/purchase_intent/purchase_intent_resource.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

namespace {
const char kUnitedStatesCountryCode[] = "kkjipiepeooghlclkedllogndmohhnhi";
}  // namespace

class BatAdsPurchaseIntentResourceTest : public UnitTestBase {
 protected:
  BatAdsPurchaseIntentResourceTest() = default;

  ~BatAdsPurchaseIntentResourceTest() override = default;
};

TEST_F(BatAdsPurchaseIntentResourceTest,
    DoNotLoadForInvalidId) {
  // Arrange
  resource::PurchaseIntent resource;

  // Act
  resource.LoadForId("invalid");

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_FALSE(is_initialized);
}

TEST_F(BatAdsPurchaseIntentResourceTest,
    LoadForId) {
  // Arrange
  resource::PurchaseIntent resource;

  // Act
  resource.LoadForId(kUnitedStatesCountryCode);

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsPurchaseIntentResourceTest,
    DoNotLoadForInvalidLocale) {
  // Arrange
  resource::PurchaseIntent resource;

  // Act
  resource.LoadForLocale("XX-XX");

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_FALSE(is_initialized);
}

TEST_F(BatAdsPurchaseIntentResourceTest,
    LoadForLocale) {
  // Arrange
  resource::PurchaseIntent resource;

  // Act
  resource.LoadForLocale("en-US");

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

}  // namespace ad_targeting
}  // namespace ads
