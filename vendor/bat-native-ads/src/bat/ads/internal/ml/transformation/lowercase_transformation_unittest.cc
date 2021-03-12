/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/transformation/lowercase_transformation.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

class BatAdsLowercaseTest : public UnitTestBase {
 protected:
  BatAdsLowercaseTest() = default;

  ~BatAdsLowercaseTest() override = default;
};

TEST_F(BatAdsLowercaseTest, LowercaseTest) {
  // Arrange
  const std::string kUppercaseStr = "LOWER CASE";
  const std::string kLowercaseStr = "lower case";
  const std::unique_ptr<Data> uppercase_data =
      std::make_unique<TextData>(kUppercaseStr);

  const LowercaseTransformation lowercase;

  // Act
  const std::unique_ptr<Data> lowercase_data = lowercase.Apply(uppercase_data);

  ASSERT_EQ(DataType::TEXT_DATA, lowercase_data->GetType());
  const TextData* lowercase_text_data =
      static_cast<TextData*>(lowercase_data.get());

  // Assert
  EXPECT_FALSE(kLowercaseStr.compare(lowercase_text_data->GetText()));
}

}  // namespace ml
}  // namespace ads
