/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_

#include <string>

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_federated/task/model.h"
#include "net/base/backoff_entry.h"

namespace brave_federated {

struct CustomBackoffEntryPolicy : net::BackoffEntry::Policy {
  static bool ParseInt64(base::StringPiece value, int64_t* field) {
    return base::StringToInt64(value, field);
  }

  static void RegisterJSONConverter(
      base::JSONValueConverter<CustomBackoffEntryPolicy>* policy_converter) {
    policy_converter->RegisterIntField(
        "num_errors_to_ignore",
        &CustomBackoffEntryPolicy::num_errors_to_ignore);
    policy_converter->RegisterIntField(
        "initial_delay_ms", &CustomBackoffEntryPolicy::initial_delay_ms);
    policy_converter->RegisterDoubleField(
        "multiply_factor", &CustomBackoffEntryPolicy::multiply_factor);
    policy_converter->RegisterDoubleField(
        "jitter_factor", &CustomBackoffEntryPolicy::jitter_factor);
    policy_converter->RegisterCustomField<int64_t>(
        "maximum_backoff_ms", &CustomBackoffEntryPolicy::maximum_backoff_ms,
        &ParseInt64);
    policy_converter->RegisterCustomField<int64_t>(
        "entry_lifetime_ms", &CustomBackoffEntryPolicy::entry_lifetime_ms,
        &ParseInt64);
    policy_converter->RegisterBoolField(
        "always_use_initial_delay",
        &CustomBackoffEntryPolicy::always_use_initial_delay);
  }

  void operator=(const net::BackoffEntry::Policy& p) {
    num_errors_to_ignore = p.num_errors_to_ignore;
    initial_delay_ms = p.initial_delay_ms;
    multiply_factor = p.multiply_factor;
    jitter_factor = p.jitter_factor;
    maximum_backoff_ms = p.maximum_backoff_ms;
    entry_lifetime_ms = p.entry_lifetime_ms;
    always_use_initial_delay = p.always_use_initial_delay;
  }
};

class LearningServiceConfig {
 public:
  LearningServiceConfig();
  explicit LearningServiceConfig(const base::FilePath& path);
  explicit LearningServiceConfig(const std::string data);
  void InitServiceConfigFromJSONString(const std::string data);
  net::BackoffEntry::Policy GetReconnectPolicy();
  net::BackoffEntry::Policy GetRequestTaskPolicy();
  net::BackoffEntry::Policy GetPostResultsPolicy();
  ModelSpec GetModelSpec();

 private:
  net::BackoffEntry::Policy reconnect_policy_;
  net::BackoffEntry::Policy request_task_policy_;
  net::BackoffEntry::Policy post_results_policy_;
  ModelSpec model_spec_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_
