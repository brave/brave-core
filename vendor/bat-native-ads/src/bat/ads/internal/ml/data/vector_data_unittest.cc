/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/data/vector_data.h"

#include <map>
#include <vector>

#include "bat/ads/internal/base/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

class BatAdsVectorDataTest : public UnitTestBase {
 protected:
  BatAdsVectorDataTest() = default;

  ~BatAdsVectorDataTest() override = default;
};

TEST_F(BatAdsVectorDataTest, DenseVectorDataInitialization) {
  // Arrange
  const std::vector<float> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_data_vector_5(v_5);

  // Act

  // Assert
  EXPECT_EQ(static_cast<int>(v_5.size()),
            dense_data_vector_5.GetDimensionCount());
}

TEST_F(BatAdsVectorDataTest, SparseVectorDataInitialization) {
  // Arrange
  const int kDimensionCount = 6;
  const std::map<unsigned, double> s_6 = {{0UL, 1.0}, {2UL, 3.0}, {3UL, -2.0}};
  const VectorData sparse_data_vector_6(kDimensionCount, s_6);

  // Act

  // Assert
  EXPECT_EQ(kDimensionCount, sparse_data_vector_6.GetDimensionCount());
}

TEST_F(BatAdsVectorDataTest, DenseDenseProduct) {
  // Arrange
  const double kTolerance = 1e-6;

  const std::vector<float> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_data_vector_5(v_5);

  const std::vector<float> v_3{1.0, 2.0, 3.0};
  const VectorData dense_data_vector_3(v_3);

  const std::vector<float> v_3_1{1.0, 1.0, 1.0};
  const VectorData dense_data_vector_3_1(v_3_1);

  // Act
  const double res_3x3 = dense_data_vector_3 * dense_data_vector_3;
  const double res_5x5 = dense_data_vector_5 * dense_data_vector_5;
  const double res_3x1 = dense_data_vector_3 * dense_data_vector_3_1;

  // Assert
  EXPECT_TRUE(std::fabs(14.0 - res_3x3) < kTolerance &&
              std::fabs(55.0 - res_5x5) < kTolerance &&
              std::fabs(6.0 - res_3x1) < kTolerance);
}

TEST_F(BatAdsVectorDataTest, SparseSparseProduct) {
  // Arrange
  const double kTolerance = 1e-6;

  // Dense equivalent is [1, 0, 2]
  const std::map<unsigned, double> s_3 = {{0UL, 1.0}, {2UL, 2.0}};
  const VectorData sparse_data_vector_3(3, s_3);

  // Dense equivalent is [1, 0, 3, 2, 0]
  const std::map<unsigned, double> s_5 = {{0UL, 1.0}, {2UL, 3.0}, {3UL, -2.0}};
  const VectorData sparse_data_vector_5(5, s_5);

  // Act
  const double res_3x3 = sparse_data_vector_3 * sparse_data_vector_3;  // = 5
  const double res_5x5 = sparse_data_vector_5 * sparse_data_vector_5;  // = 14

  // Assert
  EXPECT_TRUE(std::fabs(5.0 - res_3x3) < kTolerance &&
              std::fabs(14.0 - res_5x5) < kTolerance);
}

TEST_F(BatAdsVectorDataTest, SparseDenseProduct) {
  // Arrange
  const double kTolerance = 1e-6;

  const std::vector<float> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_data_vector_5(v_5);

  const std::vector<float> v_3{1.0, 2.0, 3.0};
  const VectorData dense_data_vector_3(v_3);

  // Dense equivalent is [1, 0, 2]
  const std::map<unsigned, double> s_3 = {{0UL, 1.0}, {2UL, 2.0}};
  const VectorData sparse_data_vector_3 = VectorData(3, s_3);

  // Dense equivalent is [1, 0, 3, 2, 0]
  const std::map<unsigned, double> s_5 = {{0UL, 1.0}, {2UL, 3.0}, {3UL, -2.0}};
  const VectorData sparse_data_vector_5(5, s_5);

  // Act
  const double mixed_res_3x3_1 =
      dense_data_vector_3 * sparse_data_vector_3;  // = 7
  const double mixed_res_5x5_1 =
      dense_data_vector_5 * sparse_data_vector_5;  // = 2
  const double mixed_res_3x3_2 =
      sparse_data_vector_3 * dense_data_vector_3;  // = 7
  const double mixed_res_5x5_2 =
      sparse_data_vector_5 * dense_data_vector_5;  // = 2

  // Assert
  EXPECT_TRUE(std::fabs(mixed_res_3x3_1 - mixed_res_3x3_2) < kTolerance &&
              std::fabs(mixed_res_5x5_1 - mixed_res_5x5_2) < kTolerance &&
              std::fabs(7.0 - mixed_res_3x3_1) < kTolerance &&
              std::fabs(2.0 - mixed_res_5x5_2) < kTolerance);
}

TEST_F(BatAdsVectorDataTest, NonsenseProduct) {
  // Arrange
  const std::vector<float> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_data_vector_5(v_5);

  const std::vector<float> v_3{1.0, 2.0, 3.0};
  const VectorData dense_data_vector_3(v_3);

  // Dense equivalent is [1, 0, 2]
  const std::map<unsigned, double> s_3 = {{0UL, 1.0}, {2UL, 2.0}};
  const VectorData sparse_data_vector_3(3, s_3);

  // Dense equivalent is [1, 0, 3, 2, 0]
  const std::map<unsigned, double> s_5 = {{0UL, 1.0}, {2UL, 3.0}, {3UL, -2.0}};
  const VectorData sparse_data_vector_5(5, s_5);

  // Act
  const double wrong_dd = dense_data_vector_5 * dense_data_vector_3;
  const double wrong_ss = sparse_data_vector_3 * sparse_data_vector_5;
  const double wrong_sd = sparse_data_vector_3 * dense_data_vector_5;
  const double wrong_ds = dense_data_vector_5 * sparse_data_vector_3;

  // Assert
  EXPECT_TRUE(std::isnan(wrong_dd) && std::isnan(wrong_ss) &&
              std::isnan(wrong_sd) && std::isnan(wrong_ds));
}

TEST_F(BatAdsVectorDataTest, Add) {
  // Arrange
  VectorData v1 = VectorData({0.3, 0.5, 0.8});
  VectorData v1_b = VectorData({0.3, 0.5, 0.8});
  VectorData v2 = VectorData({1.0, -0.6, 0.0});
  VectorData v3 = VectorData({0.0, 0.0, 0.0});
  VectorData v4 = VectorData({0.7, 0.2, -0.35});

  std::vector<float> v12({1.3, -0.1, 0.8});
  std::vector<float> v21({1.3, -0.1, 0.8});
  std::vector<float> v34({0.7, 0.2, -0.35});

  // Act
  v1.AddElementWise(v2);
  v2.AddElementWise(v1_b);
  v3.AddElementWise(v4);

  // Assert
  for (int i = 0; i < 3; i++) {
    EXPECT_NEAR(v12[i], v1.GetValuesForTesting().at(i), 0.001F);
    EXPECT_NEAR(v21[i], v2.GetValuesForTesting().at(i), 0.001F);
    EXPECT_NEAR(v34[i], v3.GetValuesForTesting().at(i), 0.001F);
  }
}

TEST_F(BatAdsVectorDataTest, ScalarDivide) {
  // Arrange
  VectorData v1 = VectorData({0.4, 0.3, 0.8});
  VectorData v2 = VectorData({1.9, -0.75, 0.0});
  VectorData v3 = VectorData({0.0, 0.0, 0.0});
  VectorData v4 = VectorData({0.8, 0.2, -0.35});

  const std::vector<float> v1d({8.0, 6.0, 16.0});
  const std::vector<float> v2d({1.9, -0.75, 0.0});
  const std::vector<float> v3d({0.0, 0.0, 0.0});
  const std::vector<float> v4d({-3.2, -0.8, 1.4});

  // Act
  v1.DivideByScalar(0.05);
  v2.DivideByScalar(1.0);
  v3.DivideByScalar(2.3);
  v4.DivideByScalar(-0.25);

  // Assert
  for (int i = 0; i < 3; i++) {
    EXPECT_NEAR(v1d[i], v1.GetValuesForTesting().at(i), 0.001F);
    EXPECT_NEAR(v2d[i], v2.GetValuesForTesting().at(i), 0.001F);
    EXPECT_NEAR(v3d[i], v3.GetValuesForTesting().at(i), 0.001F);
    EXPECT_NEAR(v4d[i], v4.GetValuesForTesting().at(i), 0.001F);
  }
}

TEST_F(BatAdsVectorDataTest, NormalizeDenseVector) {
  VectorData dense_data_vector_5({1, 3, 5, 5, 2});
  dense_data_vector_5.Normalize();
  EXPECT_EQ(std::vector<float>({1. / 8, 3. / 8, 5. / 8, 5. / 8, 2. / 8}),
            dense_data_vector_5.GetValuesForTesting());
}

TEST_F(BatAdsVectorDataTest, NormalizeSparseVector) {
  const int kDimensionCount = 6;
  const std::map<unsigned, double> s_5 = {
      {0UL, 1.0}, {2UL, 3.0}, {3UL, -2.0}, {10UL, -1.0}, {30UL, 1.0}};
  VectorData sparse_data_vector_5(kDimensionCount, s_5);
  sparse_data_vector_5.Normalize();
  EXPECT_EQ(std::vector<float>({1. / 4, 3. / 4, -2. / 4, -1. / 4, 1. / 4}),
            sparse_data_vector_5.GetValuesForTesting());
}

}  // namespace ml
}  // namespace ads
