/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/transformation/hashed_ngrams_transformation.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::ml {

class BatAdsHashedNGramsTransformationTest : public UnitTestBase {};

TEST_F(BatAdsHashedNGramsTransformationTest, HashingTest) {
  // Arrange
  constexpr int kDefaultBucketCount = 10'000;
  constexpr size_t kExpectedElementCount = 10;
  constexpr char kTestString[] = "tiny";
  const std::unique_ptr<Data> text_data =
      std::make_unique<TextData>(kTestString);

  const HashedNGramsTransformation hashed_ngrams;

  // Act
  const std::unique_ptr<Data> hashed_data = hashed_ngrams.Apply(text_data);

  ASSERT_EQ(hashed_data->GetType(), DataType::kVector);

  const VectorData* const hashed_vector_data =
      static_cast<VectorData*>(hashed_data.get());

  // Assert
  // 10000 is the default size
  ASSERT_EQ(kDefaultBucketCount, hashed_vector_data->GetDimensionCount());

  // Hashes for [t, i, n, y, ti, in, ny, tin, iny, tiny] -- 10 in total
  EXPECT_EQ(kExpectedElementCount,
            hashed_vector_data->GetValuesForTesting().size());
}

TEST_F(BatAdsHashedNGramsTransformationTest, CustomHashingTest) {
  // Arrange
  constexpr int kHashBucketCount = 3;
  constexpr char kTestString[] = "tiny";
  const std::unique_ptr<Data> text_data =
      std::make_unique<TextData>(kTestString);

  const HashedNGramsTransformation hashed_ngrams(kHashBucketCount,
                                                 std::vector<int>{1, 2, 3});

  // Act
  const std::unique_ptr<Data> hashed_data = hashed_ngrams.Apply(text_data);

  ASSERT_EQ(DataType::kVector, hashed_data->GetType());

  const VectorData* const hashed_vector_data =
      static_cast<VectorData*>(hashed_data.get());

  // Assert
  ASSERT_EQ(kHashBucketCount, hashed_vector_data->GetDimensionCount());
  EXPECT_EQ(kHashBucketCount,
            static_cast<int>(hashed_vector_data->GetValuesForTesting().size()));
}

}  // namespace ads::ml
