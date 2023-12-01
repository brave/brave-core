/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/linear_pipeline_util.h"

#include <optional>
#include <string>
#include <utility>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

namespace {
constexpr char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/linear/valid_spam_classification.fb";
}  // namespace

class BraveAdsLinearPipelineUtilTest : public UnitTestBase {};

TEST_F(BraveAdsLinearPipelineUtilTest, LoadLinearPipelineTest) {
  // Arrange
  const std::optional<std::string> contents =
      MaybeReadFileToString(kValidSpamClassificationPipeline);
  ASSERT_TRUE(contents);

  // Act & Assert
  EXPECT_TRUE(pipeline::LoadLinearPipeline(
      reinterpret_cast<const uint8_t*>(contents->data()), contents->size()));
}

}  // namespace brave_ads::ml
