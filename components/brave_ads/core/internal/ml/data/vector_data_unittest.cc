/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/data/vector_data.h"

#include <map>

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsVectorDataTest : public UnitTestBase {};

TEST_F(BraveAdsVectorDataTest, DenseVectorDataInitialization) {
  // Arrange
  const std::vector<float> vector_5{1.0F, 2.0F, 3.0F, 4.0F, 5.0F};
  const VectorData dense_vector_data_5(vector_5);

  // Act & Assert
  EXPECT_EQ(vector_5.size(), dense_vector_data_5.GetDimensionCount());
}

TEST_F(BraveAdsVectorDataTest, SparseVectorDataInitialization) {
  // Arrange
  constexpr size_t kDimensionCount = 6;

  const std::map<unsigned, double> sparse_vector_6 = {
      {0U, 1.0}, {2U, 3.0}, {3U, -2.0}};
  const VectorData sparse_vector_data_6(kDimensionCount, sparse_vector_6);

  // Act & Assert
  EXPECT_EQ(kDimensionCount, sparse_vector_data_6.GetDimensionCount());
}

TEST_F(BraveAdsVectorDataTest, DenseDenseProduct) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  const std::vector<float> vector_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_vector_data_5(vector_5);

  const std::vector<float> vector_3{1.0, 2.0, 3.0};
  const VectorData dense_vector_data_3(vector_3);

  const std::vector<float> vector_3_1{1.0, 1.0, 1.0};
  const VectorData dense_vector_data_3_1(vector_3_1);

  // Act
  const double res_3x3 = dense_vector_data_3 * dense_vector_data_3;
  const double res_5x5 = dense_vector_data_5 * dense_vector_data_5;
  const double res_3x1 = dense_vector_data_3 * dense_vector_data_3_1;

  // Assert
  EXPECT_TRUE(std::fabs(14.0 - res_3x3) < kTolerance &&
              std::fabs(55.0 - res_5x5) < kTolerance &&
              std::fabs(6.0 - res_3x1) < kTolerance);
}

TEST_F(BraveAdsVectorDataTest, SparseSparseProduct) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  // Dense equivalent is [1, 0, 2]
  const std::map<unsigned, double> sparse_vector_3 = {{0U, 1.0}, {2U, 2.0}};
  const VectorData sparse_vector_data_3(3, sparse_vector_3);

  // Dense equivalent is [1, 0, 3, 2, 0]
  const std::map<unsigned, double> sparse_vector_5 = {
      {0U, 1.0}, {2U, 3.0}, {3U, -2.0}};
  const VectorData sparse_vector_data_5(5, sparse_vector_5);

  // Act
  const double res_3x3 = sparse_vector_data_3 * sparse_vector_data_3;  // = 5
  const double res_5x5 = sparse_vector_data_5 * sparse_vector_data_5;  // = 14

  // Assert
  EXPECT_TRUE(std::fabs(5.0 - res_3x3) < kTolerance &&
              std::fabs(14.0 - res_5x5) < kTolerance);
}

TEST_F(BraveAdsVectorDataTest, SparseDenseProduct) {
  // Arrange
  constexpr double kTolerance = 1e-6;

  const std::vector<float> vector_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_vector_data_5(vector_5);

  const std::vector<float> vector_3{1.0, 2.0, 3.0};
  const VectorData dense_vector_data_3(vector_3);

  // Dense equivalent is [1, 0, 2]
  const std::map<unsigned, double> sparse_vector_3 = {{0U, 1.0}, {2U, 2.0}};
  const VectorData sparse_vector_data_3 = VectorData(3, sparse_vector_3);

  // Dense equivalent is [1, 0, 3, 2, 0]
  const std::map<unsigned, double> sparse_vector_5 = {
      {0U, 1.0}, {2U, 3.0}, {3U, -2.0}};
  const VectorData sparse_vector_data_5(5, sparse_vector_5);

  // Act
  const double mixed_res_3x3_1 =
      dense_vector_data_3 * sparse_vector_data_3;  // = 7
  const double mixed_res_5x5_1 =
      dense_vector_data_5 * sparse_vector_data_5;  // = 2
  const double mixed_res_3x3_2 =
      sparse_vector_data_3 * dense_vector_data_3;  // = 7
  const double mixed_res_5x5_2 =
      sparse_vector_data_5 * dense_vector_data_5;  // = 2

  // Assert
  EXPECT_TRUE(std::fabs(mixed_res_3x3_1 - mixed_res_3x3_2) < kTolerance &&
              std::fabs(mixed_res_5x5_1 - mixed_res_5x5_2) < kTolerance &&
              std::fabs(7.0 - mixed_res_3x3_1) < kTolerance &&
              std::fabs(2.0 - mixed_res_5x5_2) < kTolerance);
}

TEST_F(BraveAdsVectorDataTest, NonsenseProduct) {
  // Arrange
  const std::vector<float> vector_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_vector_data_5(vector_5);

  const std::vector<float> vector_3{1.0, 2.0, 3.0};
  const VectorData dense_vector_data_3(vector_3);

  // Dense equivalent is [1, 0, 2]
  const std::map<unsigned, double> sparse_vector_3 = {{0U, 1.0}, {2U, 2.0}};
  const VectorData sparse_vector_data_3(3, sparse_vector_3);

  // Dense equivalent is [1, 0, 3, 2, 0]
  const std::map<unsigned, double> sparse_vector_5 = {
      {0U, 1.0}, {2U, 3.0}, {3U, -2.0}};
  const VectorData sparse_vector_data_5(5, sparse_vector_5);

  // Act
  const double wrong_dd = dense_vector_data_5 * dense_vector_data_3;
  const double wrong_ss = sparse_vector_data_3 * sparse_vector_data_5;
  const double wrong_sd = sparse_vector_data_3 * dense_vector_data_5;
  const double wrong_ds = dense_vector_data_5 * sparse_vector_data_3;

  // Assert
  EXPECT_TRUE(std::isnan(wrong_dd) && std::isnan(wrong_ss) &&
              std::isnan(wrong_sd) && std::isnan(wrong_ds));
}

TEST_F(BraveAdsVectorDataTest, AddElementWise) {
  // Arrange
  const std::vector<float> data_1 = {0.3F, 0.5F, 0.8F};
  VectorData vector_data_1 = VectorData(data_1);
  const VectorData vector_data_1_b = VectorData(data_1);
  VectorData vector_data_2 = VectorData({1.0F, -0.6F, 0.0F});
  VectorData vector_data_3 = VectorData({0.0F, 0.0F, 0.0F});
  const VectorData vector_data_4 = VectorData({0.7F, 0.2F, -0.35F});

  const std::vector<float> vector_sum_1_2({1.3F, -0.1F, 0.8F});
  const std::vector<float> vector_sum_2_1({1.3F, -0.1F, 0.8F});
  const std::vector<float> vector_sum_3_4({0.7F, 0.2F, -0.35F});

  // Act
  vector_data_1.AddElementWise(vector_data_2);
  vector_data_2.AddElementWise(vector_data_1_b);
  vector_data_3.AddElementWise(vector_data_4);

  // Assert
  for (int i = 0; i < 3; i++) {
    EXPECT_NEAR(vector_sum_1_2.at(i), vector_data_1.GetData().at(i), 0.001F);
    EXPECT_NEAR(vector_sum_2_1.at(i), vector_data_2.GetData().at(i), 0.001F);
    EXPECT_NEAR(vector_sum_3_4.at(i), vector_data_3.GetData().at(i), 0.001F);
  }
}

TEST_F(BraveAdsVectorDataTest, DivideByScalar) {
  // Arrange
  VectorData vector_data_1 = VectorData({0.4F, 0.3F, 0.8F});
  VectorData vector_data_2 = VectorData({1.9F, -0.75F, 0.0F});
  VectorData vector_data_3 = VectorData({0.0F, 0.0F, 0.0F});
  VectorData vector_data_4 = VectorData({0.8F, 0.2F, -0.35F});

  const std::vector<float> vector_1_division({8.0F, 6.0F, 16.0F});
  const std::vector<float> vector_2_division({1.9F, -0.75F, 0.0F});
  const std::vector<float> vector_3_division({0.0F, 0.0F, 0.0F});
  const std::vector<float> vector_4_division({-3.2F, -0.8F, 1.4F});

  // Act
  vector_data_1.DivideByScalar(0.05F);
  vector_data_2.DivideByScalar(1.0F);
  vector_data_3.DivideByScalar(2.3F);
  vector_data_4.DivideByScalar(-0.25F);

  // Assert
  for (int i = 0; i < 3; i++) {
    EXPECT_NEAR(vector_1_division.at(i), vector_data_1.GetData().at(i), 0.001F);
    EXPECT_NEAR(vector_2_division.at(i), vector_data_2.GetData().at(i), 0.001F);
    EXPECT_NEAR(vector_3_division.at(i), vector_data_3.GetData().at(i), 0.001F);
    EXPECT_NEAR(vector_4_division.at(i), vector_data_4.GetData().at(i), 0.001F);
  }
}

TEST_F(BraveAdsVectorDataTest, NormalizeDenseVector) {
  VectorData dense_vector_data_5({1, 3, 5, 5, 2});
  dense_vector_data_5.Normalize();
  EXPECT_EQ(std::vector<float>(
                {1.0 / 8.0, 3.0 / 8.0, 5.0 / 8.0, 5.0 / 8.0, 2.0 / 8.0}),
            dense_vector_data_5.GetData());
}

TEST_F(BraveAdsVectorDataTest, NormalizeSparseVector) {
  constexpr size_t kDimensionCount = 6;
  const std::map<unsigned, double> sparse_vector_5 = {
      {0U, 1.0}, {2U, 3.0}, {3U, -2.0}, {10U, -1.0}, {30U, 1.0}};
  VectorData sparse_vector_data_5(kDimensionCount, sparse_vector_5);
  sparse_vector_data_5.Normalize();
  EXPECT_EQ(std::vector<float>(
                {1.0 / 4.0, 3.0 / 4.0, -2.0 / 4.0, -1.0 / 4.0, 1.0 / 4.0}),
            sparse_vector_data_5.GetData());
}

TEST_F(BraveAdsVectorDataTest, GetNorm) {
  const VectorData vector_data_1({-1.0, 1.0, 2.0, -2.0, 2.0, 1.0, 1.0});
  double norm = vector_data_1.GetNorm();
  EXPECT_EQ(4.0, norm);
}

TEST_F(BraveAdsVectorDataTest, ComputeSimilarity) {
  const VectorData vector_data_1({-1.0, 1.0, 2.0, -2.0, 2.0, 1.0, 1.0});
  const VectorData vector_data_2({-2.0, 1.0, 1.0, -1.0, 2.0, 2.0, 1.0});
  const float similarity = vector_data_1.ComputeSimilarity(vector_data_2);
  EXPECT_EQ(0.875, similarity);
}

}  // namespace brave_ads::ml
