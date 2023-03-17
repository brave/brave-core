/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/util/linear_algebra_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests -filter=BraveFederatedLearning*

namespace brave_federated {

TEST(BraveFederatedLearningLinearAlgebraUtilTest, SubtractVector) {
  // Arrange
  Vector vector_1 = {1.0, 2.0, 3.0};
  Vector vector_2 = {1.0, 2.0, 3.0};

  // Act
  Vector result = LinearAlgebraUtil::SubtractVector(vector_1, vector_2);

  // Assert
  EXPECT_EQ(3U, result.size());
  EXPECT_EQ(result[0], 0.0);
  EXPECT_EQ(result[1], 0.0);
  EXPECT_EQ(result[2], 0.0);
}

TEST(BraveFederatedLearningLinearAlgebraUtilTest, MultiplyMatrixVector) {
  // Arrange
  Matrix matrix = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}};
  Vector vector = {1.0, 2.0, 3.0};

  // Act
  std::vector<float> result =
      LinearAlgebraUtil::MultiplyMatrixVector(matrix, vector);

  // Assert
  EXPECT_EQ(3U, result.size());
  Vector expected = {14.0, 32.0, 50.0};
  EXPECT_EQ(expected, result);
}

TEST(BraveFederatedLearningLinearAlgebraUtilTest, AddVectorScalar) {
  // Arrange
  Vector vector = {1.0, 2.0, 3.0};
  float scalar = 1.0;

  // Act
  Vector result = LinearAlgebraUtil::AddVectorScalar(vector, scalar);

  // Assert
  EXPECT_EQ(3U, result.size());
  EXPECT_EQ(vector[0] + scalar, result[0]);
  EXPECT_EQ(vector[1] + scalar, result[1]);
  EXPECT_EQ(vector[2] + scalar, result[2]);
}

TEST(BraveFederatedLearningLinearAlgebraUtilTest, AddVectors) {
  // Arrange
  Vector vector_1 = {1.0, 2.0, 3.0};
  Vector vector_2 = {1.0, 2.0, 3.0};

  // Act
  Vector result = LinearAlgebraUtil::AddVectors(vector_1, vector_2);

  // Assert
  EXPECT_EQ(3U, result.size());
  EXPECT_EQ(vector_1[0] + vector_2[0], result[0]);
  EXPECT_EQ(vector_1[1] + vector_2[1], result[1]);
  EXPECT_EQ(vector_1[2] + vector_2[2], result[2]);
}

TEST(BraveFederatedLearningLinearAlgebraUtilTest, MultiplyVectorScalar) {
  // Arrange
  Vector vector = {1.0, 2.0, 3.0};
  float scalar = 2.0;

  // Act
  Vector result = LinearAlgebraUtil::MultiplyVectorScalar(vector, scalar);

  // Assert
  EXPECT_EQ(3U, result.size());
  EXPECT_EQ(vector[0] * scalar, result[0]);
  EXPECT_EQ(vector[1] * scalar, result[1]);
  EXPECT_EQ(vector[2] * scalar, result[2]);
}

TEST(BraveFederatedLearningLinearAlgebraUtilTest, TransposeMatrix) {
  // Arrange
  Matrix matrix = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};

  // Act
  Matrix result = LinearAlgebraUtil::TransposeMatrix(matrix);

  // Assert
  EXPECT_EQ(3U, result.size());
  EXPECT_EQ(2U, result.at(0).size());
  EXPECT_EQ(2U, result.at(1).size());
  EXPECT_EQ(2U, result.at(2).size());
  EXPECT_EQ(result.at(0).at(0), 1.0);
  EXPECT_EQ(result.at(0).at(1), 4.0);
  EXPECT_EQ(result.at(1).at(0), 2.0);
  EXPECT_EQ(result.at(1).at(1), 5.0);
  EXPECT_EQ(result.at(2).at(0), 3.0);
  EXPECT_EQ(result.at(2).at(1), 6.0);
}

}  // namespace brave_federated
