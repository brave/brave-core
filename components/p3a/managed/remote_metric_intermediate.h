/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_INTERMEDIATE_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_INTERMEDIATE_H_

#include <memory>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"

class TimePeriodStorage;
class PrefService;

namespace p3a {

class RemoteMetricIntermediate {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual TimePeriodStorage* GetTimePeriodStorage(
        std::string_view storage_key,
        int period_days) = 0;
    virtual void TriggerUpdate() = 0;
    virtual std::unique_ptr<RemoteMetricIntermediate> GetIntermediateInstance(
        const base::Value& config) = 0;
  };

  explicit RemoteMetricIntermediate(Delegate* delegate);
  virtual ~RemoteMetricIntermediate() = default;

  virtual bool Init() = 0;

  virtual base::Value Process() = 0;

  virtual base::flat_set<std::string_view> GetStorageKeys() const = 0;

  virtual void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) = 0;

 protected:
  raw_ptr<Delegate> delegate_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_INTERMEDIATE_H_
