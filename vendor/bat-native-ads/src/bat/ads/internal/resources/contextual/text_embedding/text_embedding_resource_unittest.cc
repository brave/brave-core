/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/contextual/text_embedding/text_embedding_resource.h"

#include <string>
#include <utility>

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_file_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::resource {

namespace {

constexpr char kResourceFile[] = "wtpwsrqtjxmfdwaymauprezkunxprysm";
constexpr char kInvalidJsonResourceFile[] =
    "resources/wtpwsrqtjxmfdwaymauprezkunxprysm_invalid_json";
constexpr char kNotExistantResourceFile[] =
    "resources/wtpwsrqtjxmfdwaymauprezkunxprysm_not_existant";

}  // namespace

using testing::_;
using testing::Invoke;

class BatAdsTextEmbeddingResourceTest : public UnitTestBase {};

TEST_F(BatAdsTextEmbeddingResourceTest, Load) {
  // Arrange
  TextEmbedding resource;

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BatAdsTextEmbeddingResourceTest, LoadInvalidJsonResource) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidJsonResourceFile, kResourceFile);

  TextEmbedding resource;
  resource.Load();

  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

TEST_F(BatAdsTextEmbeddingResourceTest, LoadNotExistantJsonResource) {
  // Arrange
  TextEmbedding resource;
  EXPECT_CALL(*ads_client_mock_, LoadFileResource(_, _, _))
      .WillOnce(Invoke([](const std::string& /*id*/, const int /*version*/,
                          LoadFileCallback callback) {
        const base::FilePath path =
            GetFileResourcePath().AppendASCII(kNotExistantResourceFile);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

TEST_F(BatAdsTextEmbeddingResourceTest, LoadNotInitializedFile) {
  // Arrange
  const TextEmbedding resource;

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

}  // namespace ads::resource
