/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/values.h"
#include "bat/ads/internal/ml/pipeline/pipeline_info.h"
#include "bat/ads/internal/ml/pipeline/pipeline_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

namespace {

const char kValidSpamClassificationPipeline[] =
    "ml/pipeline/text_processing/valid_spam_classification.json";

}  // namespace

class BatAdsPipelineUtilTest : public UnitTestBase {
 protected:
  BatAdsPipelineUtilTest() = default;

  ~BatAdsPipelineUtilTest() override = default;
};

TEST_F(BatAdsPipelineUtilTest, ParsePipelineJSONTest) {
  // Arrange
  const base::Optional<std::string> opt_value =
      ReadFileFromTestPathToString(kValidSpamClassificationPipeline);
  ASSERT_TRUE(opt_value.has_value());
  const std::string json = opt_value.value();

  // Act
  const base::Optional<pipeline::PipelineInfo> pipeline_info =
      pipeline::ParsePipelineJSON(json);

  // Assert
  EXPECT_TRUE(pipeline_info.has_value());
}

}  // namespace ml
}  // namespace ads
