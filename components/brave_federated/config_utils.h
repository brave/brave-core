/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_

#include <string>

#include "base/files/file_util.h"
#include "base/json/json_value_converter.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_federated/task/model.h"
#include "net/base/backoff_entry.h"

namespace brave_federated {
class LearningServiceConfig {
 public:
  LearningServiceConfig();
  explicit LearningServiceConfig(const base::FilePath& path);
  explicit LearningServiceConfig(const std::string& data);
  void InitServiceConfigFromJSONString(const std::string& data);
  net::BackoffEntry::Policy GetReconnectPolicy();
  net::BackoffEntry::Policy GetRequestTaskPolicy();
  net::BackoffEntry::Policy GetPostResultsPolicy();
  api::config::ModelSpec& GetModelSpec();

 private:
  net::BackoffEntry::Policy reconnect_policy_ = {};
  net::BackoffEntry::Policy request_task_policy_ = {};
  net::BackoffEntry::Policy post_results_policy_ = {};
  api::config::ModelSpec model_spec_ = {};
  void copyModelSpec(const api::config::ModelSpec& src,
                     api::config::ModelSpec& dst);
  template <typename S,typename T>
  void convertPolicy(const S& src, T& dst);
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_CONFIG_UTILS_H_
