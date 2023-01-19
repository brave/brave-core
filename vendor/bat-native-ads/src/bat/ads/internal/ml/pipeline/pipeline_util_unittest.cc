/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/pipeline_util.h"

#include <string>
#include <utility>

#include "base/test/values_test_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_file_util.h"
#include "bat/ads/internal/ml/pipeline/pipeline_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::ml {

namespace {
constexpr char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_spam_classification.json";
}  // namespace

class BatAdsPipelineUtilTest : public UnitTestBase {};

TEST_F(BatAdsPipelineUtilTest, ParsePipelineValueTest) {
  // Arrange
  const absl::optional<std::string> json =
      ReadFileFromTestPathToString(kValidSpamClassificationPipeline);
  ASSERT_TRUE(json);

  base::Value value = base::test::ParseJson(*json);

  // Act
  const absl::optional<pipeline::PipelineInfo> pipeline =
      pipeline::ParsePipelineValue(std::move(value));

  // Assert
  EXPECT_TRUE(pipeline);
}

}  // namespace ads::ml
