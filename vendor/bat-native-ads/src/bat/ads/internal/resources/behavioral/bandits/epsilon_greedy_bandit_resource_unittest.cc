/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource.h"

#include <string>

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/catalog/catalog.h"
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
  Catalog catalog;

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(json);

  const absl::optional<CatalogInfo> catalog_info =
      JSONReader::ReadCatalog(*json);
  ASSERT_TRUE(catalog_info);

  // Act
  EpsilonGreedyBandit resource(&catalog);
  resource.LoadFromCatalog(*catalog_info);

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       SuccessfullyInitializeWithEmptyCatalog) {
  // Arrange
  Catalog catalog;
  CatalogInfo catalog_info;

  // Act
  EpsilonGreedyBandit resource(&catalog);
  resource.LoadFromCatalog(catalog_info);

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BatAdsEpsilonGreedyBanditResourceTest,
       FailToInitializeIfCatalogIsNotLoaded) {
  // Arrange
  Catalog catalog;

  // Act
  EpsilonGreedyBandit resource(&catalog);

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

}  // namespace resource
}  // namespace ads
