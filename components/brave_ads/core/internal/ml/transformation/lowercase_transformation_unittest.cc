/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/transformation/lowercase_transformation.h"

#include <vector>

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"
#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsLowercaseTransformationTest : public test::TestBase {};

TEST_F(BraveAdsLowercaseTransformationTest, WrongInputDataTest) {
  // Arrange
  const std::unique_ptr<Data> vector_data =
      std::make_unique<VectorData>(std::vector<float>(1.0F));

  const LowercaseTransformation lowercase;

  // Act
  const std::unique_ptr<Data> output_data = lowercase.Apply(vector_data);

  // Assert
  EXPECT_FALSE(output_data.get());
}

TEST_F(BraveAdsLowercaseTransformationTest, LowercaseTest) {
  // Arrange
  constexpr char kUppercaseText[] = "LOWER CASE";
  constexpr char kLowercaseText[] = "lower case";
  const std::unique_ptr<Data> uppercase_data =
      std::make_unique<TextData>(kUppercaseText);

  const LowercaseTransformation lowercase;

  // Act
  const std::unique_ptr<Data> lowercase_data = lowercase.Apply(uppercase_data);

  ASSERT_EQ(DataType::kText, lowercase_data->GetType());
  const TextData* const lowercase_text_data =
      static_cast<TextData*>(lowercase_data.get());

  // Assert
  EXPECT_EQ(kLowercaseText, lowercase_text_data->GetText());
}

}  // namespace brave_ads::ml
