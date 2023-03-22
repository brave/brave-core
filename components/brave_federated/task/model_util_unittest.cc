/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/model_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveFederatedLearning*

namespace brave_federated {

TEST(BraveFederatedLearningModelUtilTest, ComputeNLL) {
  // Arrange
  std::vector<float> true_labels = {0.0, 1.0, 0.0, 1.0};
  std::vector<float> predictions = {0.1, 0.9, 0.2, 0.8};

  // Act
  float nll = ComputeNLL(true_labels, predictions);

  // Assert
  EXPECT_NEAR(0.65700f / true_labels.size(), nll, 0.001f);
}

TEST(BraveFederatedLearningModelUtilTest, SigmoidActivation) {
  // Arrange
  float z_0 = 0.0;
  float z_1 = 1.0;
  float z_2 = -1.0;

  // Act
  float a_0 = SigmoidActivation(z_0);
  float a_1 = SigmoidActivation(z_1);
  float a_2 = SigmoidActivation(z_2);

  // Assert
  EXPECT_NEAR(a_0, 0.5f, 0.001f);
  EXPECT_NEAR(a_1, 0.7310586f, 0.001f);
  EXPECT_NEAR(a_2, 0.26894143f, 0.001f);
}

}  // namespace brave_federated
