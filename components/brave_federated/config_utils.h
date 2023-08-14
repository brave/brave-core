/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_

#include <string>

#include "base/files/file_path.h"
#include "brave/components/brave_federated/api/config.h"
#include "net/base/backoff_entry.h"

namespace brave_federated {

class LearningServiceConfig {
 public:
  LearningServiceConfig();
  explicit LearningServiceConfig(const base::FilePath& path);
  explicit LearningServiceConfig(const std::string& data);
  void InitServiceConfigFromJSONString(const std::string& data);
  api::config::ModelSpec& GetModelSpec() { return model_spec_; }
  net::BackoffEntry::Policy& GetReconnectPolicy() { return reconnect_policy_; }
  net::BackoffEntry::Policy& GetRequestTaskPolicy() {
    return request_task_policy_;
  }
  net::BackoffEntry::Policy& GetPostResultsPolicy() {
    return post_results_policy_;
  }

 private:
  net::BackoffEntry::Policy reconnect_policy_;
  net::BackoffEntry::Policy request_task_policy_;
  net::BackoffEntry::Policy post_results_policy_;
  api::config::ModelSpec model_spec_;
  static void CopyModelSpec(const api::config::ModelSpec& src,
                            api::config::ModelSpec& dst);
  template <typename T, typename U>
  void ConvertPolicy(const T& src, U& dst);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_
