/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"

class PrefService;
class TimePeriodStorage;

namespace p3a {

class RemoteMetric {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void UpdateMetric(std::string_view metric_name, size_t bucket) = 0;
    virtual TimePeriodStorage* GetTimePeriodStorage(
        std::string_view storage_key,
        int period_days) = 0;
  };

  explicit RemoteMetric(Delegate* delegate, std::string_view metric_name);
  virtual ~RemoteMetric() = default;

  virtual void Init() {}

  virtual void HandleHistogramChange(std::string_view histogram_name,
                                     size_t sample) = 0;
  virtual std::vector<std::string_view> GetSourceHistogramNames() const = 0;
  virtual std::optional<std::vector<std::string_view>> GetStorageKeys()
      const = 0;

  // Called when the last used profile's preferences change
  virtual void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) {}

 protected:
  raw_ptr<Delegate> delegate_;
  std::string metric_name_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_
