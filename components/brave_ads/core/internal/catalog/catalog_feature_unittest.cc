/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCatalogFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kCatalogFeature));
}

TEST(BraveAdsCatalogFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kCatalogFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kCatalogFeature));
}

TEST(BraveAdsCatalogFeatureTest, CatalogLifespan) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(kCatalogFeature,
                                                         {{"lifespan", "2h"}});

  // Act & Assert
  EXPECT_EQ(base::Hours(2), kCatalogLifespan.Get());
}

TEST(BraveAdsCatalogFeatureTest, DefaultCatalogLifespan) {
  // Act & Assert
  EXPECT_EQ(base::Days(1), kCatalogLifespan.Get());
}

TEST(BraveAdsCatalogFeatureTest, DefaultCatalogLifespanWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kCatalogFeature);

  // Act & Assert
  EXPECT_EQ(base::Days(1), kCatalogLifespan.Get());
}

}  // namespace brave_ads
