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
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_federated/task/model_util.h"

namespace brave_federated {

namespace {

base::ThreadTicks GetThreadTicksIfSupported() {
  if (base::ThreadTicks::IsSupported()) {
    return base::ThreadTicks::Now();
  }

  return {};
}

}  // namespace

PerformanceReportInfo::PerformanceReportInfo(
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

PerformanceReportInfo::PerformanceReportInfo(
    const PerformanceReportInfo& other) = default;
PerformanceReportInfo::~PerformanceReportInfo() = default;

Model::Model(const api::config::ModelSpec& model_spec)
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

Weights Model::GetWeights() const {
  return weights_;
}

void Model::SetWeights(Weights new_weights) {
  weights_ = std::move(new_weights);
}

float Model::GetBias() const {
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

base::expected<std::vector<float>, std::string> Model::Predict(
    const DataSet& dataset) {
  if (dataset.empty()) {
    return base::unexpected("Predict input is empty");
  }

  std::vector<float> prediction(dataset.size(), 0.0);
  for (size_t i = 0; i < dataset.size(); i++) {
    if (dataset.at(i).size() != weights_.size()) {
      return base::unexpected("Predict input size mismatch in dataset");
    }

    float z = 0.0;
    for (size_t j = 0; j < dataset.at(i).size(); j++) {
      z += weights_.at(j) * dataset.at(i).at(j);
    }
    z += bias_;

    prediction.at(i) = SigmoidActivation(z);
  }

  return prediction;
}

base::expected<PerformanceReportInfo, std::string> Model::Train(
    const DataSet& train_dataset) {
  if (train_dataset.empty()) {
    return base::unexpected("Training data empty");
  }

  if (train_dataset.size() < GetBatchSize()) {
    return base::unexpected("Batch size > training dataset size");
  }

  auto data_prep_duration = base::TimeDelta();
  auto training_duration = base::TimeDelta();

  auto data_start_thread_ticks = GetThreadTicksIfSupported();

  int features = train_dataset.at(0).size() - 1;
  std::vector<float> data_indices(train_dataset.size());
  for (size_t i = 0; i < train_dataset.size(); i++) {
    data_indices.push_back(i);
  }

  Weights d_w(features);
  Weights p_w(features);
  std::vector<float> err(batch_size_, 0);
  float training_loss = 0.0;

  data_prep_duration += GetThreadTicksIfSupported() - data_start_thread_ticks;

  for (int iteration = 0; iteration < num_iterations_; iteration++) {
    data_start_thread_ticks = GetThreadTicksIfSupported();
    base::RandomShuffle(data_indices.begin(), data_indices.end());

    DataSet x(batch_size_, std::vector<float>(features));
    std::vector<float> y(batch_size_);

    auto execution_start_thread_ticks = GetThreadTicksIfSupported();
    for (int i = 0; i < batch_size_; i++) {
      std::vector<float> point = train_dataset.at(data_indices.at(i));
      y.at(i) = point.back();
      point.pop_back();
      x.at(i) = point;
    }

    p_w = weights_;
    float p_b = bias_;

    auto pred = Predict(x);
    if (!pred.has_value()) {
      std::string error_message = "Train predict failed in iteration " +
                                  base::NumberToString(iteration) + " of " +
                                  base::NumberToString(num_iterations_);
      return base::unexpected(error_message);
    }

    err = linear_algebra_util::SubtractVector(y, pred.value());

    auto transpose_matrix = linear_algebra_util::TransposeMatrix(x);
    d_w = linear_algebra_util::MultiplyMatrixVector(transpose_matrix, err);
    d_w = linear_algebra_util::MultiplyVectorScalar(d_w, (-2.0 / batch_size_));

    const float d_b =
        (-2.0 / batch_size_) * std::accumulate(err.begin(), err.end(), 0.0);

    auto update =
        linear_algebra_util::MultiplyVectorScalar(d_w, learning_rate_);
    weights_ = linear_algebra_util::SubtractVector(p_w, update);
    bias_ = p_b - learning_rate_ * d_b;

    if (iteration % 250 == 0) {
      auto loss_pred = Predict(x);
      if (!loss_pred.has_value()) {
        std::string error_message = "Train loss predict failed in iteration " +
                                    base::NumberToString(iteration) + " of " +
                                    base::NumberToString(num_iterations_);
        return base::unexpected(error_message);
      }
      training_loss = ComputeNegativeLogLikelihood(y, loss_pred.value());
    }

    data_prep_duration +=
        execution_start_thread_ticks - data_start_thread_ticks;
    training_duration +=
        GetThreadTicksIfSupported() - execution_start_thread_ticks;
  }

  float accuracy = training_loss;

  std::map<std::string, double> metrics = {
      {"data_prep_duration_in_seconds", data_prep_duration.InSecondsF()},
      {"training_duration_in_seconds", training_duration.InSecondsF()}};

  std::vector<Weights> reported_model = {weights_, {bias_}};
  return PerformanceReportInfo(train_dataset.size(),  // dataset_size
                               training_loss,         // loss
                               accuracy,              // accuracy
                               reported_model,        // parameters
                               metrics                // metrics
  );
}

base::expected<PerformanceReportInfo, std::string> Model::Evaluate(
    const DataSet& test_dataset) {
  if (test_dataset.empty()) {
    return base::unexpected("Test data empty");
  }

  auto data_start_ts = GetThreadTicksIfSupported();

  int num_features = test_dataset.at(0).size();
  DataSet features(test_dataset.size(), std::vector<float>(num_features));
  std::vector<float> ground_truth(test_dataset.size());

  for (size_t i = 0; i < test_dataset.size(); i++) {
    std::vector<float> point = test_dataset.at(i);
    ground_truth.at(i) = point.back();
    point.pop_back();
    features.at(i) = point;
  }

  auto exec_start_ts = GetThreadTicksIfSupported();

  auto predicted = Predict(features);
  if (!predicted.has_value()) {
    return base::unexpected("Evaluate predict failed");
  }

  std::vector<float> predicted_value = predicted.value();
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
  float accuracy = static_cast<float>(total_correct / test_dataset.size());

  auto loss_predicted = Predict(features);
  if (!loss_predicted.has_value()) {
    return base::unexpected("Evaluate loss predict failed");
  }
  float test_loss =
      ComputeNegativeLogLikelihood(ground_truth, loss_predicted.value());

  auto data_prep_duration = exec_start_ts - data_start_ts;
  auto evaluation_duration = GetThreadTicksIfSupported() - exec_start_ts;

  std::map<std::string, double> metrics = {
      {"data_prep_duration_in_seconds", data_prep_duration.InSecondsF()},
      {"evaluation_duration_in_seconds", evaluation_duration.InSecondsF()}};

  return PerformanceReportInfo(test_dataset.size(),  // dataset_size
                               test_loss,            // loss
                               accuracy,             // accuracy
                               {},                   // parameters
                               metrics               // metrics
  );
}

}  // namespace brave_federated
