/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_COUNTRY_CODE_COUNTRY_CODE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_COUNTRY_CODE_COUNTRY_CODE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_observer.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace brave_ads {

class CountryCode final : public AdsClientNotifierObserver,
                          public SubdivisionObserver {
 public:
  CountryCode();

  CountryCode(const CountryCode&) = delete;
  CountryCode& operator=(const CountryCode&) = delete;

  CountryCode(CountryCode&&) noexcept = delete;
  CountryCode& operator=(CountryCode&&) noexcept = delete;

  ~CountryCode() override;

 private:
  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyPrefDidChange(const std::string& path) override;

  // SubdivisionObserver:
  void OnDidUpdateSubdivision(const std::string& subdivision) override;

  void CacheCountryCode();
  void MaybeSetCountryCode();

  std::string cached_country_code_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_COUNTRY_CODE_COUNTRY_CODE_H_
