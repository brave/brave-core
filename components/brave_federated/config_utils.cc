/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/config_utils.h"

#include "base/json/json_reader.h"
#include "brave/components/brave_federated/api/config.h"
#include "brave/components/brave_federated/features.h"

namespace brave_federated {

LearningServiceConfig::LearningServiceConfig() {
  reconnect_policy_ = {.num_errors_to_ignore = 0,
                       .initial_delay_ms = 10 * 1000,
                       .multiply_factor = 2.0,
                       .jitter_factor = 0.0,
                       .maximum_backoff_ms = 60 * 10 * 1000,
                       .always_use_initial_delay = true};
  request_task_policy_ = {
      .num_errors_to_ignore = 0,
      .initial_delay_ms =
          features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
      .multiply_factor = 2.0,
      .jitter_factor = 0.0,
      .maximum_backoff_ms =
          16 * features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
      .always_use_initial_delay = true};
  post_results_policy_ = {
      .num_errors_to_ignore = 0,
      .initial_delay_ms =
          features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
      .multiply_factor = 2.0,
      .jitter_factor = 0.0,
      .maximum_backoff_ms =
          16 * features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
      .always_use_initial_delay = true};
  model_spec_.num_params = 32, model_spec_.batch_size = 32,
  model_spec_.learning_rate = 0.01, model_spec_.num_iterations = 500,
  model_spec_.threshold = 0.5;
}

LearningServiceConfig::LearningServiceConfig(const base::FilePath& path)
    : LearningServiceConfig::LearningServiceConfig() {
  std::string data;
  const bool success = base::ReadFileToString(path, &data);
  VLOG(2) << "Data: " << data;
  if (!success || data.empty()) {
    VLOG(1) << "Error in reading JSON configuration from " << path;
    return;  // return default ctor's initialisation
  }

  InitServiceConfigFromJSONString(data);
}

LearningServiceConfig::LearningServiceConfig(const std::string& data)
    : LearningServiceConfig::LearningServiceConfig() {
  InitServiceConfigFromJSONString(data);
}

void LearningServiceConfig::InitServiceConfigFromJSONString(
    const std::string& data) {
  const absl::optional<base::Value> root =
      base::JSONReader::Read(data, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

  if (!root || !root->is_dict()) {
    VLOG(1) << "Error in configuration file: root is not a dict.";
    return;  // return default ctor's initialisation
  }
  const base::Value::Dict& dict = root->GetDict();

  auto config = api::config::Config::FromValue(dict);
  if (!config) {
    VLOG(1) << "Error in configuration file: root is not a valid "
               "brave_federated::Config.";
  }
  const auto& reconnect_policy = config->reconnect_policy;
  const auto& request_task_policy = config->request_task_policy;
  const auto& post_results_policy = config->post_results_policy;
  copyModelSpec(config->model_spec, model_spec_);

  // Convert api::config::BackoffPolicy to BackoffEntry::Policy
  // parent class
  convertPolicy<api::config::BackoffPolicy, net::BackoffEntry::Policy>(
      reconnect_policy, reconnect_policy_);
  convertPolicy<api::config::BackoffPolicy, net::BackoffEntry::Policy>(
      request_task_policy, request_task_policy_);
  convertPolicy<api::config::BackoffPolicy, net::BackoffEntry::Policy>(
      post_results_policy, post_results_policy_);
}

void LearningServiceConfig::copyModelSpec(const api::config::ModelSpec& src,
                                          api::config::ModelSpec& dst) {
  dst.num_params = src.num_params;
  dst.batch_size = src.batch_size;
  dst.learning_rate = src.learning_rate;
  dst.num_iterations = src.num_iterations;
  dst.threshold = src.threshold;
}

template <typename S, typename T>
void LearningServiceConfig::convertPolicy(const S& src, T& dst) {
  dst.num_errors_to_ignore = src.num_errors_to_ignore;
  dst.initial_delay_ms = src.initial_delay_ms;
  dst.multiply_factor = src.multiply_factor;
  dst.jitter_factor = src.jitter_factor;
  bool ret =
      base::StringToInt64(src.maximum_backoff_ms, &dst.maximum_backoff_ms);
  DCHECK(ret);
  dst.always_use_initial_delay = src.always_use_initial_delay;
}

net::BackoffEntry::Policy LearningServiceConfig::GetReconnectPolicy() {
  return reconnect_policy_;
}

net::BackoffEntry::Policy LearningServiceConfig::GetRequestTaskPolicy() {
  return request_task_policy_;
}

net::BackoffEntry::Policy LearningServiceConfig::GetPostResultsPolicy() {
  return post_results_policy_;
}

api::config::ModelSpec& LearningServiceConfig::GetModelSpec() {
  return model_spec_;
}
}  // namespace brave_federated
