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
  const int default_bucket_count = 10'000;
  const size_t expected_element_count = 10;
  const std::string test_string = "tiny";
  const std::unique_ptr<Data> text_data =
      std::make_unique<TextData>(test_string);

  const HashedNGramsTransformation hashed_ngrams;

  // Act
  const std::unique_ptr<Data> hashed_data = hashed_ngrams.Apply(text_data);

  ASSERT_EQ(hashed_data->GetType(), DataType::kVector);

  const VectorData* const hashed_vector_data =
      static_cast<VectorData*>(hashed_data.get());

  // Assert
  // 10000 is the default size
  ASSERT_EQ(default_bucket_count, hashed_vector_data->GetDimensionCount());

  // Hashes for [t, i, n, y, ti, in, ny, tin, iny, tiny] -- 10 in total
  EXPECT_EQ(expected_element_count,
            hashed_vector_data->GetValuesForTesting().size());
}

TEST_F(BatAdsHashedNGramsTransformationTest, CustomHashingTest) {
  // Arrange
  const int hash_bucket_count = 3;
  const std::string test_string = "tiny";
  const std::unique_ptr<Data> text_data =
      std::make_unique<TextData>(test_string);

  const HashedNGramsTransformation hashed_ngrams(hash_bucket_count,
                                                 std::vector<int>{1, 2, 3});

  // Act
  const std::unique_ptr<Data> hashed_data = hashed_ngrams.Apply(text_data);

  ASSERT_EQ(DataType::kVector, hashed_data->GetType());

  const VectorData* const hashed_vector_data =
      static_cast<VectorData*>(hashed_data.get());

  // Assert
  ASSERT_EQ(hash_bucket_count, hashed_vector_data->GetDimensionCount());
  EXPECT_EQ(hash_bucket_count,
            static_cast<int>(hashed_vector_data->GetValuesForTesting().size()));
}

}  // namespace ads::ml
