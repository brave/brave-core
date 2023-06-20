/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/task/model.h"

#include <cmath>
#include <numeric>
#include <utility>

#include "base/check_op.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "brave/components/brave_federated/task/model_util.h"

namespace brave_federated {

PerformanceReport::PerformanceReport(
    size_t dataset_size,
    float loss,
    float accuracy,
    const std::vector<Weights>& parameters,
    const std::map<std::string, double>& metrics)
    : dataset_size(dataset_size),
      loss(loss),
      accuracy(accuracy),
      parameters(parameters),
      metrics(metrics) {}

PerformanceReport::PerformanceReport(const PerformanceReport& other) = default;
PerformanceReport::~PerformanceReport() = default;

Model::Model(const ModelSpec& model_spec)
    : num_iterations_(model_spec.num_iterations),
      batch_size_(model_spec.batch_size),
      learning_rate_(model_spec.learning_rate),
      threshold_(model_spec.threshold) {
  CHECK_GT(model_spec.num_iterations, 0);
  CHECK_GT(model_spec.batch_size, 0);

  const double max_weight = 10.0;
  const double min_weight = -10.0;

  for (int i = 0; i < model_spec.num_params; i++) {
    weights_.push_back(base::RandInt(min_weight, max_weight));
  }
  bias_ = base::RandInt(min_weight, max_weight);
}

Model::~Model() = default;

Weights Model::GetWeights() {
  return weights_;
}

void Model::SetWeights(const Weights& new_weights) {
  weights_ = std::move(new_weights);
}

float Model::GetBias() {
  return bias_;
}

void Model::SetBias(float new_bias) {
  bias_ = new_bias;
}

size_t Model::GetModelSize() const {
  return weights_.size();
}

size_t Model::GetBatchSize() const {
  return batch_size_;
}

std::vector<float> Model::Predict(const DataSet& dataset) {
  if (dataset.empty()) {
    return std::vector<float>();
  }

  std::vector<float> prediction(dataset.size(), 0.0);
  for (size_t i = 0; i < dataset.size(); i++) {
    CHECK_EQ(dataset.at(i).size(), weights_.size());

    float z = 0.0;
    for (size_t j = 0; j < dataset.at(i).size(); j++) {
      z += weights_.at(j) * dataset.at(i).at(j);
    }
    CHECK_EQ(dataset.at(i).size(), weights_.size());
    z += bias_;

    prediction.at(i) = SigmoidActivation(z);
  }

  return prediction;
}

PerformanceReport Model::Train(const DataSet& train_dataset) {
  if (train_dataset.empty()) {
    std::vector<Weights> reported_model;
    reported_model.push_back(weights_);
    reported_model.push_back({bias_});
    return PerformanceReport(0, 0.0, 0.0, reported_model, {});
  }
  CHECK_LE(GetBatchSize(), train_dataset.size());

  auto data_prep_cumulative_duration = base::TimeDelta();
  auto training_cumulative_duration = base::TimeDelta();

  auto data_start_ts = base::ThreadTicks::Now();

  int features = train_dataset.at(0).size() - 1;
  std::vector<float> data_indices(train_dataset.size());
  for (size_t i = 0; i < train_dataset.size(); i++) {
    data_indices.push_back(i);
  }

  Weights d_w(features);
  std::vector<float> err(batch_size_, 10000);
  Weights p_w(features);
  float training_loss = 0.0;

  auto data_end_ts = base::ThreadTicks::Now();

  data_prep_cumulative_duration += base::TimeDelta(data_end_ts - data_start_ts);

  for (int iteration = 0; iteration < num_iterations_; iteration++) {
    data_start_ts = base::ThreadTicks::Now();
    base::RandomShuffle(data_indices.begin(), data_indices.end());

    DataSet x(batch_size_, std::vector<float>(features));
    std::vector<float> y(batch_size_);

    auto exec_start_ts = base::ThreadTicks::Now();
    for (int i = 0; i < batch_size_; i++) {
      std::vector<float> point = train_dataset[data_indices.at(i)];
      y.at(i) = point.back();
      point.pop_back();
      x.at(i) = point;
    }

    p_w = weights_;
    float p_b = bias_;

    std::vector<float> pred = Predict(x);

    err = LinearAlgebraUtil::SubtractVector(y, pred);

    d_w = LinearAlgebraUtil::MultiplyMatrixVector(
        LinearAlgebraUtil::TransposeMatrix(x), err);
    d_w = LinearAlgebraUtil::MultiplyVectorScalar(d_w, (-2.0 / batch_size_));

    const float d_b =
        (-2.0 / batch_size_) * std::accumulate(err.begin(), err.end(), 0.0);

    weights_ = LinearAlgebraUtil::SubtractVector(
        p_w, LinearAlgebraUtil::MultiplyVectorScalar(d_w, learning_rate_));
    bias_ = p_b - learning_rate_ * d_b;

    if (iteration % 250 == 0) {
      training_loss = ComputeNegativeLogLikelihood(y, Predict(x));
    }
    auto exec_end_ts = base::ThreadTicks::Now();

    data_prep_cumulative_duration +=
        base::TimeDelta(exec_start_ts - data_start_ts);
    training_cumulative_duration +=
        base::TimeDelta(exec_end_ts - exec_start_ts);
  }

  float accuracy = training_loss;

  std::map<std::string, double> metrics = std::map<std::string, double>();
  metrics.insert({"data_prep_duration_in_seconds",
                  data_prep_cumulative_duration.InSecondsF()});
  metrics.insert({"training_duration_in_seconds",
                  training_cumulative_duration.InSecondsF()});

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
  if (test_dataset.empty()) {
    return PerformanceReport(0, 0.0, 0.0, {}, {});
  }

  auto data_start_ts = base::ThreadTicks::Now();

  int num_features = test_dataset.at(0).size();
  DataSet features(test_dataset.size(), std::vector<float>(num_features));
  std::vector<float> ground_truth(test_dataset.size());

  for (size_t i = 0; i < test_dataset.size(); i++) {
    std::vector<float> point = test_dataset.at(i);
    ground_truth.at(i) = point.back();
    point.pop_back();
    features.at(i) = point;
  }

  auto exec_start_ts = base::ThreadTicks::Now();

  std::vector<float> predicted_value = Predict(features);
  int total_correct = 0;
  for (size_t i = 0; i < test_dataset.size(); i++) {
    if (predicted_value.at(i) >= threshold_) {
      predicted_value.at(i) = 1.0;
    } else {
      predicted_value.at(i) = 0.0;
    }

    if (predicted_value.at(i) == ground_truth.at(i)) {
      total_correct++;
    }
  }
  float accuracy = total_correct * 1.0 / test_dataset.size();
  float test_loss =
      ComputeNegativeLogLikelihood(ground_truth, Predict(features));

  auto exec_end_ts = base::ThreadTicks::Now();

  auto data_prep_duration = base::TimeDelta(exec_start_ts - data_start_ts);
  auto evaluation_duration = base::TimeDelta(exec_end_ts - exec_start_ts);

  std::map<std::string, double> metrics = std::map<std::string, double>();
  metrics.insert(
      {"data_prep_duration_in_seconds", data_prep_duration.InSecondsF()});
  metrics.insert(
      {"evaluation_duration_in_seconds", evaluation_duration.InSecondsF()});

  return PerformanceReport(test_dataset.size(),  // dataset_size
                           test_loss,            // loss
                           accuracy,             // accuracy
                           {},                   // parameters
                           metrics               // metrics
  );
}

}  // namespace brave_federated
