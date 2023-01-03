/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <string>
#include <utility>

#include "base/files/file.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_file_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

using testing::_;
using testing::Invoke;

constexpr char kInvalidJsonResourceFile[] =
    "feibnmjhecfbjpeciancnchbmlobenjn_invalid_json";
constexpr char kNotExistantResourceFile[] =
    "feibnmjhecfbjpeciancnchbmlobenjn_not_existant";

}  // namespace

namespace resource {

class BatAdsTextClassificationResourceTest : public UnitTestBase {};

TEST_F(BatAdsTextClassificationResourceTest, Load) {
  // Arrange
  TextClassification resource;

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource.IsInitialized());
}

TEST_F(BatAdsTextClassificationResourceTest, LoadInvalidJsonResource) {
  // Arrange
  TextClassification resource;
  EXPECT_CALL(*ads_client_mock_, LoadFileResource(_, _, _))
      .WillOnce(Invoke([](const std::string& /*id*/, const int /*version*/,
                          LoadFileCallback callback) {
        const base::FilePath path =
            GetFileResourcePath().AppendASCII(kInvalidJsonResourceFile);

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

TEST_F(BatAdsTextClassificationResourceTest, LoadNotExistantJsonResource) {
  // Arrange
  TextClassification resource;
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

TEST_F(BatAdsTextClassificationResourceTest, LoadNotInitializedFile) {
  // Arrange
  TextClassification resource;
  EXPECT_CALL(*ads_client_mock_, LoadFileResource(_, _, _))
      .WillOnce(Invoke([](const std::string& /*id*/, const int /*version*/,
                          LoadFileCallback callback) {
        std::move(callback).Run(base::File());
      }));

  // Act
  resource.Load();
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_FALSE(resource.IsInitialized());
}

}  // namespace resource
}  // namespace ads
