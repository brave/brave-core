/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"

#include <string>

#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace resource {

namespace {
const char kCatalog[] = "catalog.json";
}  // namespace

class BatAdsEpsilonGreedyBanditResourceTest : public UnitTestBase {
 protected:
  BatAdsEpsilonGreedyBanditResourceTest() = default;

  ~BatAdsEpsilonGreedyBanditResourceTest() override = default;
};

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       SuccessfullyInitializeWithValidCatalog) {
  // Arrange
  const base::Optional<std::string> opt_value =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(opt_value.has_value());

  const std::string json = opt_value.value();

  Catalog catalog;
  ASSERT_TRUE(catalog.FromJson(json));

  // Act
  resource::EpsilonGreedyBandit resource;
  resource.LoadFromCatalog(catalog);

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       SuccessfullyInitializeWithInvalidCatalog) {
  // Arrange
  Catalog catalog;
  ASSERT_FALSE(catalog.FromJson("INVALID_CATALOG"));

  // Act
  resource::EpsilonGreedyBandit resource;
  resource.LoadFromCatalog(catalog);

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       FailToInitializeIfCatalogIsNotLoaded) {
  // Arrange

  // Act
  resource::EpsilonGreedyBandit resource;

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_FALSE(is_initialized);
}

}  // namespace resource
}  // namespace ads
