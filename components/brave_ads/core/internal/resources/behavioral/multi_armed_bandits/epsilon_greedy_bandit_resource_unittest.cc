/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/multi_armed_bandits/epsilon_greedy_bandit_resource.h"

#include <string>

#include "brave/components/brave_ads/core/internal/catalog/catalog.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_json_reader.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kCatalog[] = "catalog.json";
}  // namespace

class BraveAdsEpsilonGreedyBanditResourceTest : public UnitTestBase {};

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest,
       SuccessfullyInitializeWithCatalog) {
  // Arrange
  Catalog catalog;

  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kCatalog);
  ASSERT_TRUE(json);

  const absl::optional<CatalogInfo> catalog_info =
      json::reader::ReadCatalog(*json);
  ASSERT_TRUE(catalog_info);

  // Act
  EpsilonGreedyBanditResource resource(catalog);
  resource.LoadFromCatalog(*catalog_info);

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest,
       SuccessfullyInitializeWithEmptyCatalog) {
  // Arrange
  Catalog catalog;

  // Act
  EpsilonGreedyBanditResource resource(catalog);
  resource.LoadFromCatalog(/*catalog*/ {});

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest,
       FailToInitializeIfCatalogIsNotLoaded) {
  // Arrange
  Catalog catalog;

  // Act
  const EpsilonGreedyBanditResource resource(catalog);

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

}  // namespace brave_ads
