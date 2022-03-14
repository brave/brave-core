/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/client/model.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace brave_federated {

Model::Model(int num_iterations, float learning_rate, int num_params)
    : num_iterations_(num_iterations), learning_rate_(learning_rate) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<> distribution(-10.0, 10.0);
  for (int i = 0; i < num_params; i++) {
    prediction_weights_.push_back(distribution(mt));
  }

  prediction_bias_ = distribution(mt);
  batch_size_ = 64;
  threshold_ = 0.5;
}

Model::~Model() {}

Weights Model::GetPredWeights() {
  std::vector<float> copy_of_weights(prediction_weights_);
  return copy_of_weights;
}

void Model::SetPredWeights(Weights new_weights) {
  prediction_weights_.assign(new_weights.begin(), new_weights.end());
}

float Model::Bias() {
  return prediction_bias_;
}
void Model::SetBias(float new_bias) {
  prediction_bias_ = new_bias;
}

size_t Model::ModelSize() {
  return prediction_weights_.size();
}

std::vector<float> Model::Predict(DataSet X) {
  std::vector<float> prediction(X.size(), 0.0);
  for (size_t i = 0; i < X.size(); i++) {
    float z = 0.0;
    for (size_t j = 0; j < X[i].size(); j++) {
      z += prediction_weights_[j] * X[i][j];
    }
    z += prediction_bias_;

    prediction[i] = Activation(z);
  }
  return prediction;
}

std::tuple<size_t, float, float> Model::Train(
    const DataSet& dataset) {
  int features = dataset[0].size() - 1;

  std::vector<float> data_indices(dataset.size());
  for (size_t i = 0; i < dataset.size(); i++) {
    data_indices.push_back(i);
  }

  Weights dW(features);
  std::vector<float> err(batch_size_, 10000);
  Weights pW(features);
  float training_error = 0.0;
  for (int iteration = 0; iteration < num_iterations_; iteration++) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data_indices.begin(), data_indices.end(), g);

    DataSet X(batch_size_, std::vector<float>(features));
    std::vector<float> y(batch_size_);

    for (int i = 0; i < batch_size_; i++) {
      std::vector<float> point = dataset[data_indices[i]];
      y[i] = point.back();
      point.pop_back();
      X[i] = point;
    }

    pW = prediction_weights_;
    float pB = prediction_bias_;
    float dB;

    std::vector<float> pred = Predict(X);

    err = LinearAlgebraUtil::SubtractVector(y, pred);

    dW = LinearAlgebraUtil::MultiplyMatrixVector(
        LinearAlgebraUtil::TransposeVector(X), err);
    dW =
        LinearAlgebraUtil::MultiplyVectorScalar(dW, (-2.0 / batch_size_));

    dB = (-2.0 / batch_size_) *
         std::accumulate(err.begin(), err.end(), 0.0);

    prediction_weights_ = LinearAlgebraUtil::SubtractVector(
        pW, LinearAlgebraUtil::MultiplyVectorScalar(dW, learning_rate_));
    prediction_bias_ = pB - learning_rate_ * dB;

    if (iteration % 250 == 0) {
      training_error = ComputeNLL(y, Predict(X));
    }
  }

  float accuracy = training_error;
  return std::make_tuple(dataset.size(), training_error, accuracy);
}

float Model::ComputeNLL(std::vector<float> true_labels, std::vector<float> predictions) {
  float error = 0.0;

  for (size_t i = 0; i < true_labels.size(); i++) {
    error += (true_labels[i] * log(Activation(predictions[i])) +
              (1.0 - true_labels[i]) * log(1 - Activation(predictions[i])));
  }

  return -error;
}

float Model::Activation(float z) {
  return 1.0 / (1 + exp(-1.0 * z));
}

std::tuple<size_t, float, float> Model::Evaluate(
    const DataSet& test_dataset) {
  int num_features = test_dataset[0].size();
  DataSet X(test_dataset.size(),
                                    std::vector<float>(num_features));
  std::vector<float> y(test_dataset.size());

  for (size_t i = 0; i < test_dataset.size(); i++) {
    std::vector<float> point = test_dataset[i];
    y[i] = point.back();
    point.pop_back();
    X[i] = point;
  }

  std::vector<float> predicted_value = Predict(X);
  int total_correct = 0;
  for (size_t i = 0; i < test_dataset.size(); i++) {
    if (predicted_value[i] >= threshold_) {
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
