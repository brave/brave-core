/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_PREF_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_PREF_PROVIDER_H_

#include <optional>
#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

namespace brave_ads {

class AdsClientPrefProvider : public PrefProviderInterface {
 public:
  AdsClientPrefProvider();

  AdsClientPrefProvider(const AdsClientPrefProvider& other) = delete;
  AdsClientPrefProvider& operator=(const AdsClientPrefProvider& other) = delete;

  AdsClientPrefProvider(AdsClientPrefProvider&& other) noexcept = delete;
  AdsClientPrefProvider& operator=(AdsClientPrefProvider&& other) noexcept =
      delete;

  ~AdsClientPrefProvider() override;

  std::optional<base::Value> GetProfilePref(
      const std::string& pref_path) const override;
  bool HasProfilePrefPath(const std::string& pref_path) const override;

  std::optional<base::Value> GetLocalStatePref(
      const std::string& pref_path) const override;
  bool HasLocalStatePrefPath(const std::string& pref_path) const override;

  std::optional<base::Value> GetVirtualPref(
      const std::string& pref_path) const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_PREF_PROVIDER_H_
