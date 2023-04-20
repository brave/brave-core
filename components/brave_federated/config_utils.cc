/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <unordered_map>

#include "brave/components/brave_federated/config_utils.h"
#include "brave/components/brave_federated/features.h"
#include "brave/components/brave_federated/task/model.h"

#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"

namespace brave_federated {

LearningServiceConfig::LearningServiceConfig() {
    reconnect_policy_ = {
        /*.num_errors_to_ignore = */ 0,
        /*.initial_delay_ms = */ 10 * 1000,
        /*.multiply_factor =*/ 2.0,
        /*.jitter_factor =*/ 0.0,
        /*.maximum_backoff_ms =*/ 60 * 10 * 1000,
        /*.always_use_initial_delay =*/ true
    };
    request_task_policy_ = {
        /*.num_errors_to_ignore = */ 0,
        /*.initial_delay_ms = */ features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
        /*.multiply_factor =*/ 2.0,
        /*.jitter_factor =*/ 0.0,
        /*.maximum_backoff_ms =*/ 16 * features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
        /*.always_use_initial_delay =*/ true
    };
    post_results_policy_ = {
        /*.num_errors_to_ignore = */ 0,
        /*.initial_delay_ms = */ features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
        /*.multiply_factor =*/ 2.0,
        /*.jitter_factor =*/ 0.0,
        /*.maximum_backoff_ms =*/ 16 * features::GetFederatedLearningUpdateCycleInSeconds() * 1000,
        /*.always_use_initial_delay =*/ true
    };
    model_spec_ = {
        /* .num_params = */ 32,
        /* .batch_size = */ 32,
        /* .learning_rate = */ 0.01,
        /* .num_iterations = */ 500,
        /* .threshold = */ 0.5
    };
}

LearningServiceConfig::LearningServiceConfig(const base::FilePath& path) : LearningServiceConfig::LearningServiceConfig() {
    std::string data;
    const bool success = base::ReadFileToString(path, &data);
    if (!success || data.empty()) {
        VLOG(1) << "Error in reading JSON configuration from " << path;
        return; // return default ctor's initialisation
    }

    const absl::optional<base::Value> root = base::JSONReader::Read(data,
                                       base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);

    if (!root || !root->is_dict()) {
        VLOG(1) << "Error in configuration file (" << path << "): root is not a dict.";
        return; // return default ctor's initialisation
    }

    const base::Value::Dict& dict = root->GetDict();

    base::JSONValueConverter<CustomBackoffEntryPolicy> policy_converter;
    CustomBackoffEntryPolicy custom_reconnect_policy(reconnect_policy_);
    CustomBackoffEntryPolicy custom_request_task_policy(request_task_policy_);
    CustomBackoffEntryPolicy custom_post_results_policy(post_results_policy_);

    bool result = false;
    result = policy_converter.Convert(*(dict.Find("reconnect_policy")), &custom_reconnect_policy);
    if (not result) {
        VLOG(1) << "JSON conversion failed for reconnect policy, falling back to default values.";
    }
    result = policy_converter.Convert(*(dict.Find("request_task_policy")), &custom_request_task_policy);
    if (not result) {
        VLOG(1) << "JSON conversion failed for request policy, falling back to default values.";
    }
    result = policy_converter.Convert(*(dict.Find("post_results_policy")), &custom_post_results_policy);
    if (not result) {
        VLOG(1) << "JSON conversion failed for post results policy, falling back to default values.";
    }

    // Convert brave_federated::CustomBackoffEntryPolicy to BackoffEntry::Policy parent class
    reconnect_policy_ = custom_reconnect_policy;
    request_task_policy_ = custom_request_task_policy;
    post_results_policy_ = custom_post_results_policy;

    base::JSONValueConverter<ModelSpec> spec_converter;
    result = spec_converter.Convert(*(dict.Find("model_spec")), &model_spec_);
    if (not result) {
        VLOG(1) << "JSON conversion failed for model spec, falling back to default values.";
    }
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

ModelSpec LearningServiceConfig::GetModelSpec() {
    return model_spec_;
}
}

