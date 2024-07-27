/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/normalization_transformation.h"

#include <vector>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"
#include "brave/components/brave_ads/core/internal/ml/ml_alias.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/hashed_ngrams_transformation.h"
#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsNormalizationTransformationTest : public test::TestBase {};

TEST_F(BraveAdsNormalizationTransformationTest, WrongInputDataTest) {
  // Arrange
  constexpr char kTestString[] = "quite a small test string";
  std::unique_ptr<Data> text_data = std::make_unique<TextData>(kTestString);

  const NormalizationTransformation normalization;

  // Act
  const std::unique_ptr<Data> output_data = normalization.Apply(text_data);

  // Assert
  EXPECT_FALSE(output_data.get());
}

TEST_F(BraveAdsNormalizationTransformationTest, NormalizationTest) {
  // Arrange
  constexpr double kTolerance = 1e-7;

  constexpr char kTestString[] = "quite a small test string";
  std::unique_ptr<Data> data = std::make_unique<TextData>(kTestString);

  const HashedNGramsTransformation hashed_ngrams(
      /*bucket_count=*/10,
      /*subgrams=*/std::vector<uint32_t>{3, 4});
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

TEST_F(BraveAdsNormalizationTransformationTest, ChainingTest) {
  // Arrange
  constexpr size_t kDefaultBucketCount = 10'000;
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
  EXPECT_EQ(kDefaultBucketCount, vector_data->GetDimensionCount());
  EXPECT_THAT(vector_data->GetData(), ::testing::SizeIs(10));
}

}  // namespace brave_ads::ml
