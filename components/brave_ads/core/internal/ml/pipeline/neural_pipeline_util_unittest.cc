/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/pipeline/neural_pipeline_util.h"

#include <string>
#include <utility>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/pipeline_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

namespace {
constexpr char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_spam_classification.json";
}  // namespace

class BraveAdsPipelineUtilTest : public UnitTestBase {};

TEST_F(BraveAdsPipelineUtilTest, ParsePipelineBufferTest) {
  // Arrange
  absl::optional<std::string> buffer =
      ReadFileFromTestPathToString(kValidSpamClassificationPipeline);
  ASSERT_TRUE(buffer);

  // Act & Assert
  EXPECT_TRUE(pipeline::LoadNeuralPipeline(std::move(*buffer)));
}

}  // namespace brave_ads::ml
