/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_

#include <optional>
#include <string_view>
#include <vector>

class PrefService;

namespace p3a {

class RemoteMetric {
 public:
  virtual ~RemoteMetric() = default;

  virtual void HandleHistogramChange(std::string_view histogram_name,
                                     size_t sample) = 0;
  virtual std::vector<std::string_view> GetSourceHistogramNames() const = 0;
  virtual std::optional<std::string_view> GetStorageKey() const = 0;

  // Called when the last used profile's preferences change
  virtual void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) {}
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_REMOTE_METRIC_H_
