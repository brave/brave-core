/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_PROVIDER_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_PROVIDER_INTERFACE_H_

#include <optional>
#include <string>

#include "base/values.h"

namespace brave_ads {

inline constexpr char kVirtualPrefPathPrefix[] = "[virtual]:";

class PrefProviderInterface {
 public:
  virtual ~PrefProviderInterface() = default;

  virtual std::optional<base::Value> GetProfilePref(
      const std::string& pref_path) const = 0;
  virtual bool HasProfilePrefPath(const std::string& pref_path) const = 0;

  virtual std::optional<base::Value> GetLocalStatePref(
      const std::string& pref_path) const = 0;
  virtual bool HasLocalStatePrefPath(const std::string& pref_path) const = 0;

  virtual std::optional<base::Value> GetVirtualPref(
      const std::string& pref_path) const = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_PROVIDER_INTERFACE_H_
