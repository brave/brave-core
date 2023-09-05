/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/distribution_transformation.h"

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsDistributionTransformationTest : public UnitTestBase {};

TEST_F(BraveAdsDistributionTransformationTest, DistributionTest) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  VectorData vector_data({1.0, 2.0, 4.0, 0.03, 0.0});
  std::unique_ptr<Data> data = std::make_unique<VectorData>(vector_data);

  const DistributionTransformation to_distribution;

  // // Act
  data = to_distribution.Apply(data);
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());
  const std::vector<float> transformed_vector_values =
      transformed_vector_data->GetData();

  // Assert
  ASSERT_EQ(DataType::kVector, data->GetType());
  EXPECT_TRUE(
      (std::fabs(0.14224751 - transformed_vector_values[0]) < kTolerance) &&
      (std::fabs(0.28449502 - transformed_vector_values[1]) < kTolerance) &&
      (std::fabs(0.56899004 - transformed_vector_values[2]) < kTolerance) &&
      (std::fabs(0.00426743 - transformed_vector_values[3]) < kTolerance) &&
      (std::fabs(0.0 - transformed_vector_values[4]) < kTolerance));
}

}  // namespace brave_ads::ml
