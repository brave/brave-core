/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <vector>

#include "bat/ads/internal/ml/data/vector_data.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

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
  const std::vector<double> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
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

  const std::vector<double> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_data_vector_5(v_5);

  const std::vector<double> v_3{1.0, 2.0, 3.0};
  const VectorData dense_data_vector_3(v_3);

  const std::vector<double> v_3_1{1.0, 1.0, 1.0};
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

  const std::vector<double> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_data_vector_5(v_5);

  const std::vector<double> v_3{1.0, 2.0, 3.0};
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
  const std::vector<double> v_5{1.0, 2.0, 3.0, 4.0, 5.0};
  const VectorData dense_data_vector_5(v_5);

  const std::vector<double> v_3{1.0, 2.0, 3.0};
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

}  // namespace ml
}  // namespace ads
