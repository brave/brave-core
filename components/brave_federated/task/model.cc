/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/model.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

namespace brave_federated {

PerformanceReport::PerformanceReport(size_t dataset_size,
                                     float loss,
                                     float accuracy,
                                     std::vector<Weights> parameters,
                                     std::map<std::string, double> metrics)
    : dataset_size(dataset_size),
      loss(loss),
      accuracy(accuracy),
      parameters(parameters),
      metrics(metrics) {}

PerformanceReport::PerformanceReport(const PerformanceReport& other) = default;
PerformanceReport::~PerformanceReport() = default;

Model::Model(ModelSpec model_spec)
    : num_iterations_(model_spec.num_iterations),
      batch_size_(model_spec.batch_size),
      learning_rate_(model_spec.learning_rate),
      threshold_(model_spec.threshold) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<> distribution(-10.0, 10.0);

  for (int i = 0; i < model_spec.num_params; i++) {
    weights_.push_back(distribution(mt));
  }
  bias_ = distribution(mt);
}

Model::~Model() = default;

Weights Model::GetWeights() {
  std::vector<float> copy_of_weights(weights_);
  return copy_of_weights;
}

void Model::SetWeights(Weights new_weights) {
  weights_.assign(new_weights.begin(), new_weights.end());
}

float Model::GetBias() {
  return bias_;
}

void Model::SetBias(float new_bias) {
  bias_ = new_bias;
}

size_t Model::ModelSize() {
  return weights_.size();
}

std::vector<float> Model::Predict(const DataSet& dataset) {
  std::vector<float> prediction(dataset.size(), 0.0);
  for (size_t i = 0; i < dataset.size(); i++) {
    float z = 0.0;
    for (size_t j = 0; j < dataset[i].size(); j++) {
      z += weights_[j] * dataset[i][j];
    }
    z += bias_;

    prediction[i] = Activation(z);
  }

  return prediction;
}

PerformanceReport Model::Train(const DataSet& train_dataset) {
  double data_prep_cumulative_duration = 0;
  double train_exec_cumulative_duration = 0;

  auto data_start_ts = std::chrono::high_resolution_clock::now();

  int features = train_dataset[0].size() - 1;
  std::vector<float> data_indices(train_dataset.size());
  for (size_t i = 0; i < train_dataset.size(); i++) {
    data_indices.push_back(i);
  }

  Weights d_w(features);
  std::vector<float> err(batch_size_, 10000);
  Weights p_w(features);
  float training_loss = 0.0;

  auto data_end_ts = std::chrono::high_resolution_clock::now();

  data_prep_cumulative_duration += std::chrono::duration_cast<std::chrono::duration<double>>(data_end_ts - data_start_ts).count();

  for (int iteration = 0; iteration < num_iterations_; iteration++) {
    auto data_start_ts = std::chrono::high_resolution_clock::now();
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data_indices.begin(), data_indices.end(), g);

    DataSet x(batch_size_, std::vector<float>(features));
    std::vector<float> y(batch_size_);

    auto exec_start_ts = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < batch_size_; i++) {
      std::vector<float> point = train_dataset[data_indices[i]];
      y[i] = point.back();
      point.pop_back();
      x[i] = point;
    }

    p_w = weights_;
    float p_b = bias_;
    float d_b;

    std::vector<float> pred = Predict(x);

    err = LinearAlgebraUtil::SubtractVector(y, pred);

    d_w = LinearAlgebraUtil::MultiplyMatrixVector(
        LinearAlgebraUtil::TransposeVector(x), err);
    d_w = LinearAlgebraUtil::MultiplyVectorScalar(d_w, (-2.0 / batch_size_));

    d_b = (-2.0 / batch_size_) * std::accumulate(err.begin(), err.end(), 0.0);

    weights_ = LinearAlgebraUtil::SubtractVector(
        p_w, LinearAlgebraUtil::MultiplyVectorScalar(d_w, learning_rate_));
    bias_ = p_b - learning_rate_ * d_b;

    if (iteration % 250 == 0) {
      training_loss = ComputeNLL(y, Predict(x));
    }
    auto exec_end_ts = std::chrono::high_resolution_clock::now();

    data_prep_cumulative_duration += std::chrono::duration_cast<std::chrono::duration<double>>(exec_start_ts - data_start_ts).count();
    train_exec_cumulative_duration += std::chrono::duration_cast<std::chrono::duration<double>>(exec_end_ts - exec_start_ts).count();
  }

  float accuracy = training_loss;

  std::map<std::string, double> metrics = std::map<std::string, double>();
  metrics.insert({"data_prep_duration", data_prep_cumulative_duration});
  metrics.insert({"train_duration", train_exec_cumulative_duration});

  std::vector<Weights> reported_model;
  reported_model.push_back(weights_);
  reported_model.push_back({bias_});
  return PerformanceReport(train_dataset.size(),  // dataset_size
                           training_loss,         // loss
                           accuracy,              // accuracy
                           reported_model,        // parameters
                           metrics                // metrics
  );
}

PerformanceReport Model::Evaluate(const DataSet& test_dataset) {
  auto data_start_ts = std::chrono::high_resolution_clock::now();

  int num_features = test_dataset[0].size();
  DataSet X(test_dataset.size(), std::vector<float>(num_features));
  std::vector<float> y(test_dataset.size());

  for (size_t i = 0; i < test_dataset.size(); i++) {
    std::vector<float> point = test_dataset[i];
    y[i] = point.back();
    point.pop_back();
    X[i] = point;
  }

  auto exec_start_ts = std::chrono::high_resolution_clock::now();

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

  auto exec_end_ts = std::chrono::high_resolution_clock::now();

  auto data_prep_duration = std::chrono::duration_cast<std::chrono::duration<double>>(exec_start_ts - data_start_ts).count();
  auto train_exec_duration = std::chrono::duration_cast<std::chrono::duration<double>>(exec_end_ts - exec_start_ts).count();

  std::map<std::string, double> metrics = std::map<std::string, double>();
  metrics.insert({"data_prep_duration", data_prep_duration});
  metrics.insert({"eval_duration", train_exec_duration});

  return PerformanceReport(test_dataset.size(),  // dataset_size
                           test_loss,            // loss
                           accuracy,             // accuracy
                           {},                   // parameters
                           metrics               // metrics
  );
}

float Model::ComputeNLL(std::vector<float> true_labels,
                        std::vector<float> predictions) {
  float error = 0.0;

  for (size_t i = 0; i < true_labels.size(); i++) {
    error += (true_labels[i] * log(predictions[i]) +
              (1.0 - true_labels[i]) * log(1 - predictions[i]));
  }

  return -error;
}

float Model::Activation(float z) {
  return 1.0 / (1 + exp(-1.0 * z));
}

}  // namespace brave_federated
