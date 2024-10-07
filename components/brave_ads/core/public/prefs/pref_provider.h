/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_PROVIDER_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

class PrefService;

namespace brave_ads {

class PrefProvider : public PrefProviderInterface {
 public:
  explicit PrefProvider(PrefService* profile_prefs,
                        PrefService* local_state_prefs);

  PrefProvider(const PrefProvider& other) = delete;
  PrefProvider& operator=(const PrefProvider& other) = delete;

  PrefProvider(PrefProvider&& other) noexcept = delete;
  PrefProvider& operator=(PrefProvider&& other) noexcept = delete;

  ~PrefProvider() override;

  std::optional<base::Value> GetProfilePref(
      const std::string& pref_path) const override;
  bool HasProfilePrefPath(const std::string& pref_path) const override;

  std::optional<base::Value> GetLocalStatePref(
      const std::string& pref_path) const override;
  bool HasLocalStatePrefPath(const std::string& pref_path) const override;

 private:
  const raw_ptr<PrefService> profile_prefs_ = nullptr;
  const raw_ptr<PrefService> local_state_prefs_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_PROVIDER_H_
