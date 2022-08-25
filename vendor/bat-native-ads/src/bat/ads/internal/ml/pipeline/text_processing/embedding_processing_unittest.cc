/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/pipeline/text_processing/embedding_processing.h"

#include <map>
#include <vector>

#include "base/files/file.h"
#include "base/time/time.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/resources/contextual/text_embedding/text_embedding_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using testing::_;
using testing::Invoke;

namespace {

constexpr char kSimpleResourceFile[] =
    "wtpwsrqtjxmfdwaymauprezkunxprysm_simple";

}  // namespace

namespace ads {

class BatAdsEmbeddingProcessingPipelineTest : public UnitTestBase {
 protected:
  BatAdsEmbeddingProcessingPipelineTest() = default;

  ~BatAdsEmbeddingProcessingPipelineTest() override = default;
};

TEST_F(BatAdsEmbeddingProcessingPipelineTest, EmbedTextSimple) {
  // Arrange
  resource::TextEmbedding resource;
  EXPECT_CALL(*ads_client_mock_, LoadFileResource(_, _, _))
      .WillOnce(Invoke([](const std::string& id, const int version,
                          LoadFileCallback callback) {
        const base::FilePath path =
            GetFileResourcePath().AppendASCII(kSimpleResourceFile);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));

  resource.Load();
  task_environment_.RunUntilIdle();

  EXPECT_TRUE(resource.IsInitialized());
  ml::pipeline::EmbeddingProcessing* embedding_processing = resource.Get();

  const std::map<std::string, ml::VectorData> samples = {
      {"this simple unittest", ml::VectorData({0.5, 0.4, 1.0})},
      {"this is a simple unittest", ml::VectorData({0.5, 0.4, 1.0})},
      {"that is a test", ml::VectorData({0.0, 0.0, 0.0})},
      {"this 54 is simple", ml::VectorData({0.85, 0.2, 1.0})},
      {"", ml::VectorData({0.0, 0.0, 0.0})}};

  for (const auto& sample : samples) {
    // Act
    ml::pipeline::TextEmbeddingInfo embedding_info =
        embedding_processing->EmbedText(sample.first);
    // Assert
    EXPECT_EQ(sample.second.GetValuesForTesting(),
              embedding_info.embedding.GetValuesForTesting());
  }
}

}  // namespace ads
