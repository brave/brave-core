/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "base/version.h"
#include "brave/components/p3a/managed/remote_metric_intermediate.h"

class PrefService;
class TimePeriodStorage;

namespace p3a {

class RemoteMetricIntermediate;

// Manages a remotely-defined P3A metric. Makes use of RemoteMetricIntermediates
// to construct and report the metric value.
class RemoteMetric : public RemoteMetricIntermediate::Delegate {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void UpdateMetric(std::string_view metric_name, size_t bucket) = 0;
    virtual TimePeriodStorage* GetTimePeriodStorage(
        std::string_view storage_key,
        int period_days) = 0;
  };

  RemoteMetric(PrefService* local_state,
               PrefService* profile_prefs,
               Delegate* delegate,
               std::string_view metric_name,
               std::unique_ptr<base::Value> definition);
  ~RemoteMetric() override;

  bool Init(const base::Version& current_version);

  base::flat_set<std::string_view> GetStorageKeys() const;

  // Called when the last used profile's preferences change
  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs);

  // RemoteMetricIntermediate::Delegate implementation:
  TimePeriodStorage* GetTimePeriodStorage(std::string_view storage_key,
                                          int period_days) override;
  void TriggerUpdate() override;
  std::unique_ptr<RemoteMetricIntermediate> GetIntermediateInstance(
      const base::Value& config) override;

 private:
  raw_ptr<Delegate> delegate_;
  std::string metric_name_;
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;
  std::unique_ptr<base::Value> definition_;
  std::unique_ptr<RemoteMetricIntermediate> intermediate_;
  base::WallClockTimer daily_timer_;

  // True if last used profile prefs update is in progress.
  bool profile_pref_update_in_progress_ = false;
  // True if an update will be triggered after the last used profile prefs
  // update.
  bool update_pending_ = false;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_
