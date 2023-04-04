/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_error_or.h"

namespace brave_ads::resource {

class AntiTargeting final : public AdsClientNotifierObserver {
 public:
  AntiTargeting();

  AntiTargeting(const AntiTargeting&) = delete;
  AntiTargeting& operator=(const AntiTargeting&) = delete;

  AntiTargeting(AntiTargeting&&) noexcept = delete;
  AntiTargeting& operator=(AntiTargeting&&) noexcept = delete;

  ~AntiTargeting() override;

  bool IsInitialized() const { return is_initialized_; }

  void Load();

  const AntiTargetingInfo& get() const { return anti_targeting_; }

 private:
  void OnLoadAndParseResource(ParsingErrorOr<AntiTargetingInfo> result);

  // AdsClientNotifierObserver:
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyDidUpdateResourceComponent(const std::string& id) override;

  bool is_initialized_ = false;

  AntiTargetingInfo anti_targeting_;

  base::WeakPtrFactory<AntiTargeting> weak_factory_{this};
};

}  // namespace brave_ads::resource

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_
