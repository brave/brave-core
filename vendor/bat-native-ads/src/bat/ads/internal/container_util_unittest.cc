/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/container_util.h"

#include <deque>
#include <map>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsContainerUtilTest, VectorToDeque) {
  // Arrange
  const std::vector<std::string> vector = {"item 1", "item 2", "item 3",
                                           "item 4", "item 5", "item 6"};

  // Act
  const std::deque<std::string> deque = VectorToDeque(vector);

  // Assert
  const std::deque<std::string> expected_deque = {"item 1", "item 2", "item 3",
                                                  "item 4", "item 5", "item 6"};

  EXPECT_EQ(expected_deque, deque);
}

TEST(BatAdsContainerUtilTest, EmptyVectorToDeque) {
  // Arrange
  const std::vector<std::string> vector = {};

  // Act
  const std::deque<std::string> deque = VectorToDeque(vector);

  // Assert
  const std::deque<std::string> expected_deque = {};

  EXPECT_EQ(expected_deque, deque);
}

TEST(BatAdsContainerUtilTest, SplitVectorIntoSingleChunk) {
  // Arrange
  const std::vector<std::string> vector = {"item 1", "item 2", "item 3",
                                           "item 4", "item 5", "item 6"};

  // Act
  const std::vector<std::vector<std::string>> vectors = SplitVector(vector, 6);

  // Assert
  const std::vector<std::vector<std::string>> expected_vectors = {vectors};

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
  const std::vector<std::vector<std::string>> expected_vectors = {vectors};

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
  const std::vector<std::string> vector = {};

  // Act
  const std::vector<std::vector<std::string>> vectors = SplitVector(vector, 5);

  // Assert
  const std::vector<std::vector<std::string>> expected_vectors = {vectors};

  EXPECT_EQ(expected_vectors, vectors);
}

TEST(BatAdsContainerUtilTest, CompareMatchingMaps) {
  // Arrange
  const std::map<std::string, std::string> map_1 = {{"key 1", "value 1"},
                                                    {"key 2", "value 2"}};

  const std::map<std::string, std::string> map_2 = {{"key 2", "value 2"},
                                                    {"key 1", "value 1"}};

  // Act
  const bool does_equal = CompareMaps(map_1, map_2);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsContainerUtilTest, CompareIdenticalMatchingMaps) {
  // Arrange
  const std::map<std::string, std::string> map = {{"key 1", "value 1"},
                                                  {"key 2", "value 2"}};

  // Act
  const bool does_equal = CompareMaps(map, map);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsContainerUtilTest, CompareNonMatchingMaps) {
  // Arrange
  const std::map<std::string, std::string> map_1 = {{"key 1", "value 1"},
                                                    {"key 2", "value 2"}};

  const std::map<std::string, std::string> map_2 = {{"key 3", "value 3"},
                                                    {"key 4", "value 4"}};

  // Act
  const bool does_equal = CompareMaps(map_1, map_2);

  // Assert
  EXPECT_FALSE(does_equal);
}

TEST(BatAdsContainerUtilTest, CompareEmptyMaps) {
  // Arrange
  const std::map<std::string, std::string> map = {};

  // Act
  const bool does_equal = CompareMaps(map, map);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsContainerUtilTest, CompareMatchingSets) {
  // Arrange
  const std::deque<std::string> deque_1 = {"deque 1", "deque 2"};

  const std::deque<std::string> deque_2 = {"deque 2", "deque 1"};

  // Act
  const bool does_equal = CompareAsSets(deque_1, deque_2);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsContainerUtilTest, CompareIdenticalMatchingSets) {
  // Arrange
  const std::deque<std::string> deque = {"deque 1", "deque 2"};

  // Act
  const bool does_equal = CompareAsSets(deque, deque);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsContainerUtilTest, CompareNonMatchingSets) {
  // Arrange
  const std::deque<std::string> deque_1 = {"deque 1", "deque 2"};

  const std::deque<std::string> deque_2 = {"deque 3", "deque 4"};

  // Act
  const bool does_equal = CompareAsSets(deque_1, deque_2);

  // Assert
  EXPECT_FALSE(does_equal);
}

TEST(BatAdsContainerUtilTest, CompareEmptySets) {
  // Arrange
  const std::deque<std::string> deque = {};

  // Act
  const bool does_equal = CompareAsSets(deque, deque);

  // Assert
  EXPECT_TRUE(does_equal);
}

TEST(BatAdsContainerUtilTest, DoesNotMatchEmptySegments) {
  // Arrange
  const std::vector<std::string> lhs;
  const std::vector<std::string> rhs;

  // Act
  const std::vector<std::string> set_intersection = SetIntersection(lhs, rhs);

  // Assert
  const std::vector<std::string> expected_set_intersection;
  EXPECT_EQ(expected_set_intersection, set_intersection);
}

TEST(BatAdsContainerUtilTest, DoesNotMatchSegments) {
  // Arrange
  const std::vector<std::string> lhs = {"element 1", "element 2"};
  const std::vector<std::string> rhs = {"element 3"};

  // Act
  const std::vector<std::string> set_intersection = SetIntersection(lhs, rhs);

  // Assert
  const std::vector<std::string> expected_set_intersection;
  EXPECT_EQ(expected_set_intersection, set_intersection);
}

TEST(BatAdsContainerUtilTest, SetIntersectionForUnsortedList) {
  // Arrange
  const std::vector<std::string> lhs = {"element 1", "element 3", "element 2"};
  const std::vector<std::string> rhs = {"element 2", "element 1"};

  // Act
  const std::vector<std::string> set_intersection = SetIntersection(lhs, rhs);

  // Assert
  const std::vector<std::string> expected_set_intersection = {"element 1",
                                                              "element 2"};
  EXPECT_EQ(expected_set_intersection, set_intersection);
}

}  // namespace ads
