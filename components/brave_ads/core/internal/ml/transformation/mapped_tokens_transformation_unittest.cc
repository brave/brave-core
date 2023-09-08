/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/mapped_tokens_transformation.h"

#include <cstddef>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsMappedTokensTransformationTest : public UnitTestBase {};

TEST_F(BraveAdsMappedTokensTransformationTest, MappedTokens) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  constexpr char kTestString[] = "this is a simple test string";
  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  int vector_dimension = 6;
  std::map<std::string, std::vector<int>> token_categories_mapping = {
      {"is", {1}}, {"this", {5}}, {"test-string", {0, 3}}, {"simple", {1, 4}}};

  const MappedTokensTransformation to_mapped_tokens(vector_dimension,
                                                    token_categories_mapping);

  // // Act
  data = to_mapped_tokens.Apply(data);
  const VectorData* const transformed_vector_data =
      static_cast<VectorData*>(data.get());

  std::vector<float> transformed_vector_values(
      transformed_vector_data->GetDimensionCount());
  transformed_vector_values =
      transformed_vector_data->GetData(transformed_vector_values);

  // Assert
  ASSERT_EQ(DataType::kVector, data->GetType());
  ASSERT_TRUE(transformed_vector_values.size() ==
              static_cast<size_t>(vector_dimension));
  EXPECT_TRUE((std::fabs(1.0 - transformed_vector_values.at(0)) < kTolerance) &&
              (std::fabs(2.0 - transformed_vector_values.at(1)) < kTolerance) &&
              (std::fabs(0.0 - transformed_vector_values.at(2)) < kTolerance) &&
              (std::fabs(1.0 - transformed_vector_values.at(3)) < kTolerance) &&
              (std::fabs(1.0 - transformed_vector_values.at(4)) < kTolerance) &&
              (std::fabs(1.0 - transformed_vector_values.at(5)) < kTolerance));
}

}  // namespace brave_ads::ml
