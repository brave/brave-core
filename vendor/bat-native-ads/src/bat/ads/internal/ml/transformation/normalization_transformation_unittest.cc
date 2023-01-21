/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/transformation/normalization_transformation.h"

#include <string>
#include <vector>

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/ml_alias.h"
#include "bat/ads/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "bat/ads/internal/ml/transformation/lowercase_transformation.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::ml {

class BatAdsNormalizationTransformationTest : public UnitTestBase {};

TEST_F(BatAdsNormalizationTransformationTest, NormalizationTest) {
  // Arrange
  const double tolerance = 1e-7;

  const std::string test_string = "quite a small test string";
  std::unique_ptr<Data> data = std::make_unique<TextData>(test_string);

  const HashedNGramsTransformation hashed_ngrams(10, std::vector<int>{3, 4});
  const NormalizationTransformation normalization;

  // Act
  data = hashed_ngrams.Apply(data);

  data = normalization.Apply(data);

  ASSERT_EQ(DataType::kVector, data->GetType());

  const VectorData* const norm_data = static_cast<VectorData*>(data.release());

  std::vector<double> components;
  double s = 0.0;
  for (const double x : norm_data->GetValuesForTesting()) {
    components.push_back(x);
    s += x * x;
  }

  // Assert
  for (double const& x : components) {
    ASSERT_TRUE(x >= 0.0);
    ASSERT_TRUE(x <= 1.0);
  }
  EXPECT_TRUE(std::fabs(s - 1.0) < tolerance);
}

TEST_F(BatAdsNormalizationTransformationTest, ChainingTest) {
  // Arrange
  const int default_bucket_count = 10'000;
  const size_t expected_element_count = 10;
  const std::string test_string = "TINY";

  TransformationVector chain;

  chain.push_back(std::make_unique<LowercaseTransformation>());

  chain.push_back(std::make_unique<HashedNGramsTransformation>());

  chain.push_back(std::make_unique<NormalizationTransformation>());

  std::unique_ptr<Data> data = std::make_unique<TextData>(test_string);

  // Act
  for (auto& entry : chain) {
    data = entry->Apply(data);
  }

  ASSERT_EQ(DataType::kVector, data->GetType());
  const VectorData* const vector_data = static_cast<VectorData*>(data.get());
  ASSERT_TRUE(vector_data);

  // Assert
  ASSERT_EQ(default_bucket_count, vector_data->GetDimensionCount());

  // Hashes for [t, i, n, y, ti, in, ny, tin, iny, tiny] -- 10 in total
  EXPECT_EQ(expected_element_count, vector_data->GetValuesForTesting().size());
}

}  // namespace ads::ml
