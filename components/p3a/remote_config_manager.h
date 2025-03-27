/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_REMOTE_CONFIG_MANAGER_H_
#define BRAVE_COMPONENTS_P3A_REMOTE_CONFIG_MANAGER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"
#include "brave/components/p3a/metric_config.h"
#include "brave/components/p3a/metric_log_type.h"

namespace brave_component_updater {
class LocalDataFilesService;
}

namespace p3a {

// The RemoteConfigManager loads remote configuration data from the Brave Local
// Data component and provides this configuration as needed.
class RemoteConfigManager
    : public brave_component_updater::LocalDataFilesObserver {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual std::optional<MetricLogType> GetLogTypeForHistogram(
        std::string_view histogram_name) const = 0;
    virtual const MetricConfig* GetBaseMetricConfig(
        std::string_view histogram_name) const = 0;
  };

  RemoteConfigManager(
      brave_component_updater::LocalDataFilesService* local_data_files_service,
      Delegate* delegate);
  ~RemoteConfigManager() override;

  // brave_component_updater::LocalDataFilesObserver:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  const MetricConfig* GetRemoteMetricConfig(std::string_view metric_name) const;

 private:
  void LoadRemoteConfig(const base::FilePath& install_dir);
  void SetMetricConfigs(
      std::unique_ptr<base::flat_map<std::string, RemoteMetricConfig>> result);

  base::flat_map<std::string, MetricConfig> metric_configs_;
  base::flat_set<std::string> activation_metric_names_;

  raw_ptr<Delegate> delegate_ = nullptr;

  base::WeakPtrFactory<RemoteConfigManager> weak_factory_{this};
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_REMOTE_CONFIG_MANAGER_H_
