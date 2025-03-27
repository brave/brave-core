/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/remote_config_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/json/json_reader.h"
#include "base/json/json_value_converter.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/p3a/metric_config.h"

constexpr char kP3AManifestFileName[] = "p3a_manifest.json";
constexpr char kP3AManifestFileVersion[] = "1";

constexpr char kMetricsKey[] = "metrics";

namespace p3a {

namespace {

// Reads and parses the p3a_manifest.json file from disk
std::unique_ptr<base::flat_map<std::string, RemoteMetricConfig>>
ReadAndParseJsonRules(const base::FilePath& txt_file_path) {
  const auto raw_contents =
      brave_component_updater::GetDATFileAsString(txt_file_path);

  if (raw_contents.empty()) {
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

  auto remote_metric_configs =
      std::make_unique<base::flat_map<std::string, RemoteMetricConfig>>();

  for (const auto [metric_name, config_value] : *metrics_dict) {
    if (!config_value.is_dict()) {
      VLOG(1) << "Metric config for " << metric_name << " is not a dictionary";
      continue;
    }

    if (!converter.Convert(config_value,
                           &(*remote_metric_configs)[metric_name])) {
      VLOG(1) << "Failed to convert metric config for " << metric_name;
      remote_metric_configs->erase(metric_name);
      continue;
    }
  }

  return remote_metric_configs;
}

}  // namespace

RemoteConfigManager::RemoteConfigManager(
    brave_component_updater::LocalDataFilesService* local_data_files_service,
    Delegate* delegate)
    : brave_component_updater::LocalDataFilesObserver(local_data_files_service),
      delegate_(delegate) {}

RemoteConfigManager::~RemoteConfigManager() = default;

void RemoteConfigManager::OnComponentReady(const std::string& component_id,
                                           const base::FilePath& install_dir,
                                           const std::string& manifest) {
  LoadRemoteConfig(install_dir);
}

void RemoteConfigManager::LoadRemoteConfig(const base::FilePath& install_dir) {
  base::FilePath txt_file_path =
      install_dir.AppendASCII(kP3AManifestFileVersion)
          .AppendASCII(kP3AManifestFileName);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadAndParseJsonRules, txt_file_path),
      base::BindOnce(&RemoteConfigManager::SetMetricConfigs,
                     weak_factory_.GetWeakPtr()));
}

void RemoteConfigManager::SetMetricConfigs(
    std::unique_ptr<base::flat_map<std::string, RemoteMetricConfig>> result) {
  if (!result) {
    return;
  }

  VLOG(1) << "Loaded " << result->size() << " metric configurations";

  activation_metric_names_.clear();

  for (const auto& entry : *result) {
    if (!delegate_->GetLogTypeForHistogram(entry.first)) {
      continue;
    }

    const auto* base_config = delegate_->GetBaseMetricConfig(entry.first);

    const auto& remote_config = entry.second;

    auto metric_config = base_config ? *base_config : MetricConfig{};

    metric_config.ephemeral =
        remote_config.ephemeral.value_or(metric_config.ephemeral);
    metric_config.constellation_only =
        remote_config.constellation_only.value_or(
            metric_config.constellation_only);
    metric_config.nebula = remote_config.nebula.value_or(metric_config.nebula);
    metric_config.disable_country_strip =
        remote_config.disable_country_strip.value_or(
            metric_config.disable_country_strip);
    metric_config.record_activation_date =
        remote_config.record_activation_date.value_or(
            metric_config.record_activation_date);

    if (remote_config.attributes) {
      metric_config.attributes = remote_config.attributes;
    }
    if (remote_config.append_attributes) {
      metric_config.append_attributes = *remote_config.append_attributes;
    }
    if (remote_config.activation_metric_name) {
      auto it =
          activation_metric_names_.insert(*remote_config.activation_metric_name)
              .first;
      metric_config.activation_metric_name = *it;
    }
    if (remote_config.cadence) {
      metric_config.cadence = remote_config.cadence;
    }

    metric_configs_.emplace(entry.first, metric_config);
  }
}

const MetricConfig* RemoteConfigManager::GetRemoteMetricConfig(
    std::string_view metric_name) const {
  auto it = metric_configs_.find(metric_name);
  if (it == metric_configs_.end()) {
    return nullptr;
  }
  return &it->second;
}

}  // namespace p3a
