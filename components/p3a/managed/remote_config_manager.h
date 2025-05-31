/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_CONFIG_MANAGER_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_CONFIG_MANAGER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/p3a/metric_config.h"
#include "brave/components/p3a/metric_log_type.h"

namespace p3a {

class RemoteMetricManager;

inline constexpr base::FilePath::CharType kP3AManifestFileName[] =
    FILE_PATH_LITERAL("p3a_manifest.json");

// The RemoteConfigManager loads remote configuration data from the Brave Local
// Data component and provides this configuration as needed.
class RemoteConfigManager {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    virtual std::optional<MetricLogType> GetLogTypeForHistogram(
        std::string_view histogram_name) const = 0;
    virtual const MetricConfig* GetBaseMetricConfig(
        std::string_view histogram_name) const = 0;
    virtual void OnRemoteConfigLoaded() = 0;
  };

  RemoteConfigManager(Delegate* delegate,
                      RemoteMetricManager* remote_metric_manager);
  ~RemoteConfigManager();

  void LoadRemoteConfig(const base::FilePath& install_dir);

  const MetricConfig* GetRemoteMetricConfig(std::string_view metric_name) const;

  base::WeakPtr<RemoteConfigManager> GetWeakPtr();

  bool is_loaded() const { return is_loaded_; }

 private:
  void SetMetricConfigs(
      std::unique_ptr<
          base::flat_map<std::string, std::unique_ptr<RemoteMetricConfig>>>
          result);

  base::flat_map<std::string, MetricConfig> metric_configs_;
  base::flat_set<std::string> activation_metric_names_;

  bool is_loaded_ = false;

  raw_ptr<Delegate> delegate_ = nullptr;
  raw_ptr<RemoteMetricManager> remote_metric_manager_ = nullptr;

  base::WeakPtrFactory<RemoteConfigManager> weak_factory_{this};
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_CONFIG_MANAGER_H_
