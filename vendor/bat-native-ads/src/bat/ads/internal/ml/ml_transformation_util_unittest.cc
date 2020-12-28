/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <map>
#include <memory>
#include <string>

#include "bat/ads/internal/ml/ml_transformation_util.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

class BatAdsMLTransformationUtilTest : public UnitTestBase {
 protected:
  BatAdsMLTransformationUtilTest() = default;

  ~BatAdsMLTransformationUtilTest() override = default;
};

TEST_F(BatAdsMLTransformationUtilTest, TransformationCopyTest) {
  // Arrange
  const NormalizationTransformation normalization;
  TransformationPtr transformation_ptr =
      std::make_unique<NormalizationTransformation>(normalization);

  // Act
  const TransformationPtr transformation_ptr_copy =
      GetTransformationCopy(transformation_ptr);

  // Assert
  EXPECT_EQ(transformation_ptr_copy->GetType(),
            TransformationType::NORMALIZATION);
}

TEST_F(BatAdsMLTransformationUtilTest, TransformationVectorDeepCopyTest) {
  // Arrange
  const size_t kVectorSize = 2;

  TransformationVector transformation_vector;
  const HashedNGramsTransformation hashed_ngrams;
  transformation_vector.push_back(
      std::make_unique<HashedNGramsTransformation>(hashed_ngrams));

  const NormalizationTransformation normalization;
  transformation_vector.push_back(
      std::make_unique<NormalizationTransformation>(normalization));

  // Act
  const TransformationVector transformation_vector_copy =
      GetTransformationVectorDeepCopy(transformation_vector);

  // Assert
  ASSERT_EQ(kVectorSize, transformation_vector_copy.size());
  EXPECT_TRUE(transformation_vector_copy[0]->GetType() ==
                  TransformationType::HASHED_NGRAMS &&
              transformation_vector_copy[1]->GetType() ==
                  TransformationType::NORMALIZATION);
}

}  // namespace ml
}  // namespace ads
