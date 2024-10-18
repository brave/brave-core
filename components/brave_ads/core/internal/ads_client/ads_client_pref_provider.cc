/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_pref_provider.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

AdsClientPrefProvider::AdsClientPrefProvider() {
  virtual_prefs_ = GetAdsClient()->GetVirtualPrefs();
}

AdsClientPrefProvider::~AdsClientPrefProvider() = default;

std::optional<base::Value> AdsClientPrefProvider::GetProfilePref(
    const std::string& pref_path) const {
  if (!GetAdsClient()->FindProfilePref(pref_path)) {
    // The preference does not exist.
    return std::nullopt;
  }

  return GetAdsClient()->GetProfilePref(pref_path);
}

bool AdsClientPrefProvider::HasProfilePrefPath(
    const std::string& pref_path) const {
  if (!GetAdsClient()->FindProfilePref(pref_path)) {
    // The preference does not exist.
    return false;
  }

  return GetAdsClient()->HasProfilePrefPath(pref_path);
}

std::optional<base::Value> AdsClientPrefProvider::GetLocalStatePref(
    const std::string& pref_path) const {
  if (!GetAdsClient()->FindLocalStatePref(pref_path)) {
    // The preference does not exist.
    return std::nullopt;
  }

  return GetAdsClient()->GetLocalStatePref(pref_path);
}

bool AdsClientPrefProvider::HasLocalStatePrefPath(
    const std::string& pref_path) const {
  if (!GetAdsClient()->FindLocalStatePref(pref_path)) {
    // The preference does not exist.
    return false;
  }

  return GetAdsClient()->HasLocalStatePrefPath(pref_path);
}

std::optional<base::Value> AdsClientPrefProvider::GetVirtualPref(
    const std::string& virtual_pref_path) const {
  if (virtual_pref_path.starts_with("[virtual]:")) {
    const std::string pref_path =
        virtual_pref_path.substr(/*pos=*/std::size("[virtual]:"));

    if (const base::Value* const value = virtual_prefs_.Find(pref_path)) {
      return value->Clone();
    }
  }

  if (virtual_pref_path.starts_with("[virtual_default]:")) {
    const std::string pref_path =
        virtual_pref_path.substr(/*pos=*/std::size("[virtual_default]:"));

    if (HasProfilePrefPath(pref_path)) {
      return GetProfilePref(pref_path);
    }

    if (HasLocalStatePrefPath(pref_path)) {
      return GetLocalStatePref(pref_path);
    }

    if (const base::Value* const value = virtual_prefs_.Find(pref_path)) {
      return value->Clone();
    }
  }

  // Unknown virtual pref path.
  return std::nullopt;
}

}  // namespace brave_ads
