/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_MANAGER_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_MANAGER_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/p3a/managed/remote_metric.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace p3a {

// Manages metrics defined via remote configuration. Responsible for creating
// and tracking appropriate metric objects based on definitions received from
// the remote config system.
class RemoteMetricManager {
 public:
  using UnparsedDefinitionsMap =
      base::flat_map<std::string, std::unique_ptr<base::Value::Dict>>;

  // Delegate interface for RemoteMetricManager
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when a metric value needs to be updated
    virtual void UpdateMetricValue(
        std::string_view histogram_name,
        size_t bucket,
        std::optional<bool> only_update_for_constellation) = 0;
  };

  RemoteMetricManager(PrefService* local_state, Delegate* delegate);
  ~RemoteMetricManager();

  RemoteMetricManager(const RemoteMetricManager&) = delete;
  RemoteMetricManager& operator=(const RemoteMetricManager&) = delete;

  // Set the profile preferences service for the last used profile
  void HandleProfileLoad(PrefService* profile_prefs,
                         const base::FilePath& context_path);

  // Clear the stored PrefService if the context path matches
  void HandleProfileUnload(const base::FilePath& context_path);

  // Process all metric definitions in a map. Called by RemoteConfigManager
  // when new definitions are available.
  void ProcessMetricDefinitions(const UnparsedDefinitionsMap& definitions);

 private:
  friend class P3ARemoteMetricManagerUnitTest;

  FRIEND_TEST_ALL_PREFIXES(P3ARemoteMetricManagerUnitTest,
                           ProcessMetricDefinitions);
  FRIEND_TEST_ALL_PREFIXES(P3ARemoteMetricManagerUnitTest,
                           InvalidMetricDefinitionsAreSkipped);
  FRIEND_TEST_ALL_PREFIXES(P3ARemoteMetricManagerUnitTest,
                           ProfilePrefsHandling);
  FRIEND_TEST_ALL_PREFIXES(P3ARemoteMetricManagerUnitTest, CleanupStorage);
  FRIEND_TEST_ALL_PREFIXES(P3ARemoteMetricManagerUnitTest, MinVersionAccepted);
  FRIEND_TEST_ALL_PREFIXES(P3ARemoteMetricManagerUnitTest, MinVersionRejected);

  // Called by ScopedHistogramSampleObserver when a histogram changes.
  void OnHistogramChanged(std::string_view histogram_name,
                          uint64_t name_hash,
                          base::HistogramBase::Sample32 sample);
  void SetupObservers();
  void CleanupStorage();

  void OnLastUsedProfileChanged();

  void OnMetricUpdated(std::string_view metric_name, size_t bucket);

  // Maps from metric name to metric objects
  std::vector<std::unique_ptr<RemoteMetric>> metrics_;

  // Maps from histogram name to the list of metrics that consume it
  base::flat_map<std::string, std::vector<raw_ptr<RemoteMetric>>>
      histogram_to_metrics_;

  // Maps from histogram name to the observer watching it.
  base::flat_map<
      std::string,
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      remote_metric_sample_callbacks_;

  base::flat_map<base::FilePath, raw_ptr<PrefService>> profile_prefs_map_;
  raw_ptr<PrefService> last_used_profile_prefs_ = nullptr;

  raw_ptr<PrefService> local_state_;
  raw_ptr<Delegate> delegate_;

  PrefChangeRegistrar last_used_profile_pref_change_registrar_;

  // Version used for min_version comparisons. Can be overridden in tests.
  base::Version current_version_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_MANAGER_H_
