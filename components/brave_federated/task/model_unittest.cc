/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/model.h"
#include <cstddef>

#include "brave/components/brave_federated/util/linear_algebra_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveFederatedLearning*

namespace brave_federated {

ModelSpec g_k_model_spec = {
    64,   // num_params
    32,   // batch_size
    0.1,  // learning_rate
    10,   // num_iterations
    0.7   // threshold
};

TEST(BraveFederatedLearningModelTest, GetModelSize) {
  // Arrange
  Model model(g_k_model_spec);

  // Act
  size_t model_size = model.GetModelSize();

  // Assert
  EXPECT_EQ(static_cast<int>(model_size), g_k_model_spec.num_params);
}

TEST(BraveFederatedLearningModelTest, GetModelSizeAfterSettingWeights) {
  // Arrange
  Model model(g_k_model_spec);
  Weights weights = {1.0, 2.0, 3.0, 4.0, 5.0};

  // Act
  model.SetWeights(weights);
  size_t model_size = model.GetModelSize();

  // Assert
  EXPECT_EQ(model_size, weights.size());
}

TEST(BraveFederatedLearningModelTest, GetBatchSize) {
  // Arrange
  Model model(g_k_model_spec);
  Weights weights = {1.0, 2.0, 3.0, 4.0, 5.0};

  // Act

  // Assert
  EXPECT_EQ(static_cast<int>(model.GetBatchSize()), g_k_model_spec.batch_size);
}

}  // namespace brave_federated
