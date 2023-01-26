/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/containers/container_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsContainerUtilTest, SplitVectorIntoSingleChunk) {
  // Arrange
  const std::vector<std::string> vector = {"item 1", "item 2", "item 3",
                                           "item 4", "item 5", "item 6"};

  // Act
  const std::vector<std::vector<std::string>> vectors = SplitVector(vector, 6);

  // Assert
  const std::vector<std::vector<std::string>>& expected_vectors = {vectors};

  EXPECT_EQ(expected_vectors, vectors);
}

TEST(BatAdsContainerUtilTest,
     SplitVectorIntoSingleChunkWhenChunkSizeIsLargerThanVectorSize) {
  // Arrange
  const std::vector<std::string> vector = {"item 1", "item 2", "item 3",
                                           "item 4", "item 5", "item 6"};

  // Act
  const std::vector<std::vector<std::string>> vectors = SplitVector(vector, 7);

  // Assert
  const std::vector<std::vector<std::string>> expected_vectors = {vector};

  EXPECT_EQ(expected_vectors, vectors);
}

TEST(BatAdsContainerUtilTest, SplitVectorIntoMultipleEvenChunks) {
  // Arrange
  const std::vector<std::string> vector = {
      "item 1", "item 2", "item 3", "item 4", "item 5", "item 6",
  };

  // Act
  const std::vector<std::vector<std::string>> vectors = SplitVector(vector, 3);

  // Assert
  const std::vector<std::vector<std::string>> expected_vectors = {
      {"item 1", "item 2", "item 3"}, {"item 4", "item 5", "item 6"}};

  EXPECT_EQ(expected_vectors, vectors);
}

TEST(BatAdsContainerUtilTest, SplitVectorIntoMultipleUnevenChunks) {
  // Arrange
  const std::vector<std::string> vector = {"item 1", "item 2", "item 3",
                                           "item 4", "item 5"};

  // Act
  const std::vector<std::vector<std::string>> vectors = SplitVector(vector, 3);

  // Assert
  const std::vector<std::vector<std::string>> expected_vectors = {
      {"item 1", "item 2", "item 3"}, {"item 4", "item 5"}};

  EXPECT_EQ(expected_vectors, vectors);
}

TEST(BatAdsContainerUtilTest, SplitEmptyVector) {
  // Arrange
  const std::vector<std::string> vector;

  // Act
  const std::vector<std::vector<std::string>> vectors = SplitVector(vector, 5);

  // Assert
  const std::vector<std::vector<std::string>>& expected_vectors = {vectors};

  EXPECT_EQ(expected_vectors, vectors);
}

}  // namespace ads
