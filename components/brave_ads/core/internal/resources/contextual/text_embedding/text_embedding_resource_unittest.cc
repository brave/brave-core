/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/text_embedding_resource.h"

#include <string>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/resources/resources_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using testing::_;
using testing::Invoke;

namespace {
constexpr char kResourceId[] = "wtpwsrqtjxmfdwaymauprezkunxprysm";
}  // namespace

class BraveAdsTextEmbeddingResourceTest : public UnitTestBase {};

TEST_F(BraveAdsTextEmbeddingResourceTest, LoadResource) {
  // Arrange
  TextEmbeddingResource resource;

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidResourceId, kResourceId);

  TextEmbeddingResource resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest, DoNotLoadMissingResource) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, LoadFileResource(kResourceId, _, _))
      .WillOnce(Invoke([](const std::string& /*id*/, const int /*version*/,
                          LoadFileCallback callback) {
        const base::FilePath path =
            GetFileResourcePath().AppendASCII(kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));

  TextEmbeddingResource resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest, IsNotInitialized) {
  // Arrange
  const TextEmbeddingResource resource;

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

}  // namespace brave_ads
