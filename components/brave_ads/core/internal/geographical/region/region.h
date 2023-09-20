/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHICAL_REGION_REGION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHICAL_REGION_REGION_H_

#include <memory>
#include <string>

#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision_observer.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class Region final : public AdsClientNotifierObserver,
                     public SubdivisionObserver {
 public:
  Region();

  Region(const Region&) = delete;
  Region& operator=(const Region&) = delete;

  Region(Region&&) noexcept = delete;
  Region& operator=(Region&&) noexcept = delete;

  ~Region() override;

 private:
  // AdsClientNotifierObserver:
  void OnNotifyPrefDidChange(const std::string& path) override;

  // SubdivisionObserver:
  void OnDidUpdateSubdivision(const std::string& subdivision) override;

  void UpdateCachedRegionCode();
  void MaybeSetRegionCodePref();

  std::string cached_region_code_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GEOGRAPHICAL_REGION_REGION_H_
