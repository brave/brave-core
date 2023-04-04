/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::ml {

class BatAdsNormalizationTransformationTest : public UnitTestBase {};

TEST_F(BatAdsNormalizationTransformationTest, NormalizationTest) {
  // Arrange
  constexpr double kTolerance = 1e-7;

  constexpr char kTestString[] = "quite a small test string";
  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  const HashedNGramsTransformation hashed_ngrams(10, std::vector<int>{3, 4});
  const NormalizationTransformation normalization;

  // Act
  data = hashed_ngrams.Apply(data);

  data = normalization.Apply(data);

  ASSERT_EQ(DataType::kVector, data->GetType());

  const VectorData* const vector_data =
      static_cast<VectorData*>(data.release());

  std::vector<double> components;
  double s = 0.0;
  for (const double component : vector_data->GetData()) {
    components.push_back(component);
    s += component * component;
  }

  // Assert
  for (double const& component : components) {
    ASSERT_GE(component, 0.0);
    ASSERT_LE(component, 1.0);
  }
  EXPECT_LT(std::fabs(s - 1.0), kTolerance);
}

TEST_F(BatAdsNormalizationTransformationTest, ChainingTest) {
  // Arrange
  constexpr int kDefaultBucketCount = 10'000;
  constexpr size_t kExpectedElementCount = 10;
  constexpr char kTestString[] = "TINY";

  TransformationVector chain;

  chain.push_back(std::make_unique<LowercaseTransformation>());

  chain.push_back(std::make_unique<HashedNGramsTransformation>());

  chain.push_back(std::make_unique<NormalizationTransformation>());

  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  // Act
  for (auto& entry : chain) {
    data = entry->Apply(data);
  }

  ASSERT_EQ(DataType::kVector, data->GetType());
  const VectorData* const vector_data = static_cast<VectorData*>(data.get());
  ASSERT_TRUE(vector_data);

  // Assert
  ASSERT_EQ(kDefaultBucketCount, vector_data->GetDimensionCount());

  // Hashes for [t, i, n, y, ti, in, ny, tin, iny, tiny] -- 10 in total
  EXPECT_EQ(kExpectedElementCount, vector_data->GetData().size());
}

}  // namespace brave_ads::ml
