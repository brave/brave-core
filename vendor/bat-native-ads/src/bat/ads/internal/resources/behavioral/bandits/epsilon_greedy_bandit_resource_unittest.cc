/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"

#include <string>

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_file_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "bat/ads/internal/catalog/catalog_json_reader.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace resource {

namespace {
constexpr char kCatalog[] = "catalog.json";
}  // namespace

class BatAdsEpsilonGreedyBanditResourceTest : public UnitTestBase {
 protected:
  BatAdsEpsilonGreedyBanditResourceTest() = default;

  ~BatAdsEpsilonGreedyBanditResourceTest() override = default;
};

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       SuccessfullyInitializeWithCatalog) {
  // Arrange
  const absl::optional<std::string> json_optional =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(json_optional.has_value());
  const std::string& json = json_optional.value();

  const absl::optional<CatalogInfo> catalog_optional =
      JSONReader::ReadCatalog(json);
  ASSERT_TRUE(catalog_optional);
  const CatalogInfo& catalog = catalog_optional.value();

  // Act
  EpsilonGreedyBandit resource;
  resource.LoadFromCatalog(catalog);

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       SuccessfullyInitializeWithEmptyCatalog) {
  // Arrange
  CatalogInfo catalog;

  // Act
  EpsilonGreedyBandit resource;
  resource.LoadFromCatalog(catalog);

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       FailToInitializeIfCatalogIsNotLoaded) {
  // Arrange

  // Act
  EpsilonGreedyBandit resource;

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_FALSE(is_initialized);
}

}  // namespace resource
}  // namespace ads
