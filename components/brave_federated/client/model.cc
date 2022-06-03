/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/client/model.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "brave/components/brave_federated/linear_algebra_util/linear_algebra_util.h"

namespace brave_federated {

Model::Model(int num_iterations, float learning_rate, int num_params)
    : num_iterations_(num_iterations), learning_rate_(learning_rate) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<> distr(-10.0, 10.0);
  for (int i = 0; i < num_params; i++) {
    this->pred_weights_.push_back(distr(mt));
  }

  this->pred_b_ = distr(mt);
  this->batch_size_ = 64;
  this->threshold_ = 0.5;
}

Model::~Model() {}

std::vector<float> Model::PredWeights() {
  std::vector<float> copy_of_weights(this->pred_weights_);
  return copy_of_weights;
}

void Model::SetPredWeights(std::vector<float> new_weights) {
  this->pred_weights_.assign(new_weights.begin(), new_weights.end());
}

float Model::Bias() {
  return this->pred_b_;
}
void Model::SetBias(float new_bias) {
  this->pred_b_ = new_bias;
}

size_t Model::ModelSize() {
  return this->pred_weights_.size();
}

std::vector<float> Model::Predict(std::vector<std::vector<float>> X) {
  std::vector<float> prediction(X.size(), 0.0);
  for (int i = 0; i < (int)X.size(); i++) {
    float z = 0.0;
    for (int j = 0; j < (int)X[i].size(); j++) {
      z += this->pred_weights_[j] * X[i][j];
    }
    z += this->pred_b_;

    prediction[i] = this->Activation(z);
  }
  return prediction;
}

std::tuple<size_t, float, float> Model::Train(
    std::vector<std::vector<float>>& dataset) {
  int features = dataset[0].size() - 1;

  std::vector<float> data_indices(dataset.size());
  for (int i = 0; i < (int)dataset.size(); i++) {
    data_indices.push_back(i);
  }

  std::vector<float> dW(features);
  std::vector<float> err(batch_size_, 10000);
  std::vector<float> pW(features);
  float training_error = 0.0;
  for (int iteration = 0; iteration < num_iterations_; iteration++) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data_indices.begin(), data_indices.end(), g);

    std::vector<std::vector<float>> X(this->batch_size_,
                                      std::vector<float>(features));
    std::vector<float> y(this->batch_size_);

    for (int i = 0; i < this->batch_size_; i++) {
      std::vector<float> point = dataset[data_indices[i]];
      y[i] = point.back();
      point.pop_back();
      X[i] = point;
    }

    pW = this->pred_weights_;
    float pB = this->pred_b_;
    float dB;

    std::vector<float> pred = Predict(X);

    err = LinearAlgebraUtil::SubtractVector(y, pred);

    dW = LinearAlgebraUtil::MultiplyMatrixVector(
        LinearAlgebraUtil::TransposeVector(X), err);
    dW =
        LinearAlgebraUtil::MultiplyVectorScalar(dW, (-2.0 / this->batch_size_));

    dB = (-2.0 / this->batch_size_) *
         std::accumulate(err.begin(), err.end(), 0.0);

    this->pred_weights_ = LinearAlgebraUtil::SubtractVector(
        pW, LinearAlgebraUtil::MultiplyVectorScalar(dW, learning_rate_));
    this->pred_b_ = pB - learning_rate_ * dB;

    if (iteration % 250 == 0) {
      training_error = this->ComputeNLL(y, Predict(X));
    }
  }

  float accuracy = training_error;
  return std::make_tuple(dataset.size(), training_error, accuracy);
}

float Model::ComputeNLL(std::vector<float> true_y, std::vector<float> pred) {
  float error = 0.0;

  for (int i = 0; i < (int) true_y.size(); i++) {
    error += (true_y[i] * log(Activation(pred[i])) + (1.0 - true_y[i]) * log(1 - Activation(pred[i])));
  }

  return -error;
}

float Model::Activation(float z) {
  return 1.0 / (1 + exp(-1.0 * z));
}

std::tuple<size_t, float, float> Model::Evaluate(
    std::vector<std::vector<float>>& test_dataset) {
  int num_features = test_dataset[0].size();
  std::vector<std::vector<float>> X(test_dataset.size(),
                                    std::vector<float>(num_features));
  std::vector<float> y(test_dataset.size());

  for (int i = 0; i < (int)test_dataset.size(); i++) {
    std::vector<float> point = test_dataset[i];
    y[i] = point.back();
    point.pop_back();
    X[i] = point;
  }

  std::vector<float> predicted_value = Predict(X);
  int total_correct = 0;
  for (int i = 0; i < (int)test_dataset.size(); i++) {
    if (predicted_value[i] >= this->threshold_) {
      predicted_value[i] = 1.0;
    } else {
      predicted_value[i] = 0.0;
    }

    if (predicted_value[i] == y[i]) {
      total_correct++;
    }
  }
  float accuracy = total_correct * 1.0 / test_dataset.size();
  float test_loss = ComputeNLL(y, Predict(X));
  return std::make_tuple(test_dataset.size(), test_loss, accuracy);
}

}  // namespace brave_federated