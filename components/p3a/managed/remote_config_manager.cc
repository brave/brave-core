/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_config_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric_manager.h"
#include "brave/components/p3a/metric_config.h"
#include "brave/components/p3a/metric_config_utils.h"

constexpr char kMetricsKey[] = "metrics";

namespace p3a {

namespace {

// Reads and parses the p3a_manifest.json file from disk
std::unique_ptr<
    base::flat_map<std::string, std::unique_ptr<RemoteMetricConfig>>>
ReadAndParseJsonRules(const base::FilePath& manifest_file_path) {
  std::string raw_contents;
  if (!base::ReadFileToString(manifest_file_path, &raw_contents)) {
    VLOG(1) << "Failed to read p3a manifest";
    return nullptr;
  }

  const auto json_root = base::JSONReader::Read(raw_contents);
  if (!json_root) {
    VLOG(1) << "Failed to parse p3a manifest";
    return nullptr;
  }

  if (!json_root->is_dict()) {
    VLOG(1) << "Expected dictionary in p3a manifest";
    return nullptr;
  }

  const auto* metrics_dict = json_root->GetDict().FindDict(kMetricsKey);
  if (!metrics_dict) {
    VLOG(1) << "No metrics found in p3a manifest";
    return nullptr;
  }

  base::JSONValueConverter<RemoteMetricConfig> converter;

  auto remote_metric_configs = std::make_unique<
      base::flat_map<std::string, std::unique_ptr<RemoteMetricConfig>>>();

  for (const auto [metric_name, config_value] : *metrics_dict) {
    if (!config_value.is_dict()) {
      VLOG(1) << "Metric config for " << metric_name << " is not a dictionary";
      continue;
    }

    auto remote_config = std::make_unique<RemoteMetricConfig>();

    if (!converter.Convert(config_value, remote_config.get())) {
      VLOG(1) << "Failed to convert metric config for " << metric_name;
      continue;
    }

    remote_metric_configs->emplace(metric_name, std::move(remote_config));
  }

  return remote_metric_configs;
}

}  // namespace

RemoteConfigManager::RemoteConfigManager(
    Delegate* delegate,
    RemoteMetricManager* remote_metric_manager)
    : delegate_(delegate), remote_metric_manager_(remote_metric_manager) {
  CHECK(delegate_);
}

RemoteConfigManager::~RemoteConfigManager() = default;

void RemoteConfigManager::LoadRemoteConfig(const base::FilePath& install_dir) {
  base::FilePath manifest_file_path = install_dir.Append(kP3AManifestFileName);
  VLOG(1) << "Loading remote config";

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadAndParseJsonRules, manifest_file_path),
      base::BindOnce(&RemoteConfigManager::SetMetricConfigs,
                     weak_factory_.GetWeakPtr()));
}

void RemoteConfigManager::SetMetricConfigs(
    std::unique_ptr<
        base::flat_map<std::string, std::unique_ptr<RemoteMetricConfig>>>
        result) {
  if (!result) {
    is_loaded_ = true;
    delegate_->OnRemoteConfigLoaded();
    return;
  }

  VLOG(1) << "Loaded " << result->size() << " metric configurations";

  activation_metric_names_.clear();
  metric_configs_.clear();

  RemoteMetricManager::UnparsedDefinitionsMap metric_definitions;

  for (const auto& entry : *result) {
    // If the metric is not present in metric_names.h (checked via delegate),
    // and if there is no remote definition, skip it
    if (!delegate_->GetLogTypeForHistogram(entry.first) &&
        !entry.second->definition) {
      continue;
    }

    const auto* base_config = delegate_->GetBaseMetricConfig(entry.first);
    const auto& remote_config = entry.second;
    auto metric_config = base_config ? *base_config : MetricConfig{};

    metric_config.ephemeral =
        remote_config->ephemeral.value_or(metric_config.ephemeral);
    metric_config.constellation_only =
        remote_config->constellation_only.value_or(
            metric_config.constellation_only);
    metric_config.nebula = remote_config->nebula.value_or(metric_config.nebula);
    metric_config.disable_country_strip =
        remote_config->disable_country_strip.value_or(
            metric_config.disable_country_strip);
    metric_config.record_activation_date =
        remote_config->record_activation_date.value_or(
            metric_config.record_activation_date);

    if (remote_config->attributes) {
      metric_config.attributes = remote_config->attributes;
    }
    if (remote_config->append_attributes) {
      metric_config.append_attributes = *remote_config->append_attributes;
    }
    if (remote_config->activation_metric_name) {
      auto it = activation_metric_names_
                    .insert(*remote_config->activation_metric_name)
                    .first;
      metric_config.activation_metric_name = *it;
    }
    if (remote_config->cadence) {
      metric_config.cadence = remote_config->cadence;
    }

    metric_configs_.emplace(entry.first, metric_config);

    if (remote_config->definition) {
      metric_definitions.emplace(entry.first,
                                 std::move(remote_config->definition));
    }
  }

  if (remote_metric_manager_) {
    remote_metric_manager_->ProcessMetricDefinitions(metric_definitions);
  }

  is_loaded_ = true;
  delegate_->OnRemoteConfigLoaded();
}

const MetricConfig* RemoteConfigManager::GetRemoteMetricConfig(
    std::string_view metric_name) const {
  return base::FindOrNull(metric_configs_, metric_name);
}

base::WeakPtr<RemoteConfigManager> RemoteConfigManager::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace p3a
