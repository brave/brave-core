/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/distribution_transformation.h"

#include <vector>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsDistributionTransformationTest : public test::TestBase {};

TEST_F(BraveAdsDistributionTransformationTest, ToDistribution) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  VectorData vector_data({1.0, 2.0, 4.0, 0.03, 0.0});
  std::unique_ptr<Data> data = std::make_unique<VectorData>(vector_data);

  const DistributionTransformation to_distribution;

  // Act
  data = to_distribution.Apply(data);
  ASSERT_EQ(DataType::kVector, data->GetType());
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());
  const std::vector<float> transformed_vector_values =
      transformed_vector_data->GetData();

  // Assert
  EXPECT_THAT(transformed_vector_values, ::testing::SizeIs(5));
  EXPECT_LT(std::fabs(0.14224751 - transformed_vector_values.at(0)),
            kTolerance);
  EXPECT_LT(std::fabs(0.28449502 - transformed_vector_values.at(1)),
            kTolerance);
  EXPECT_LT(std::fabs(0.56899004 - transformed_vector_values.at(2)),
            kTolerance);
  EXPECT_LT(std::fabs(0.00426743 - transformed_vector_values.at(3)),
            kTolerance);
  EXPECT_LT(std::fabs(0.0 - transformed_vector_values.at(4)), kTolerance);
}

TEST_F(BraveAdsDistributionTransformationTest, ZeroVector) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  VectorData vector_data({0, 0, 0, 0, 0});
  std::unique_ptr<Data> data = std::make_unique<VectorData>(vector_data);

  const DistributionTransformation to_distribution;

  // Act
  data = to_distribution.Apply(data);
  ASSERT_EQ(DataType::kVector, data->GetType());
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());
  const std::vector<float> transformed_vector_values =
      transformed_vector_data->GetData();
  ASSERT_THAT(transformed_vector_values, ::testing::SizeIs(5));

  // Assert
  EXPECT_LT(std::fabs(0.0 - transformed_vector_values.at(0)), kTolerance);
  EXPECT_LT(std::fabs(0.0 - transformed_vector_values.at(1)), kTolerance);
  EXPECT_LT(std::fabs(0.0 - transformed_vector_values.at(2)), kTolerance);
  EXPECT_LT(std::fabs(0.0 - transformed_vector_values.at(3)), kTolerance);
  EXPECT_LT(std::fabs(0.0 - transformed_vector_values.at(4)), kTolerance);
}

TEST_F(BraveAdsDistributionTransformationTest, EmptyVector) {
  // Arrange
  VectorData vector_data;
  std::unique_ptr<Data> data = std::make_unique<VectorData>(vector_data);

  const DistributionTransformation to_distribution;

  // Act
  data = to_distribution.Apply(data);
  ASSERT_EQ(DataType::kVector, data->GetType());
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());
  const std::vector<float> transformed_vector_values =
      transformed_vector_data->GetData();

  // Assert
  EXPECT_THAT(transformed_vector_values, ::testing::IsEmpty());
}

TEST_F(BraveAdsDistributionTransformationTest, NonVectorData) {
  // Arrange
  constexpr char kTestString[] = "this is a test string";
  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  const DistributionTransformation to_distribution;

  // Act
  data = to_distribution.Apply(data);

  // Assert
  EXPECT_FALSE(data);
}

}  // namespace brave_ads::ml
