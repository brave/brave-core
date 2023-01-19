/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/locale/locale_manager_observer.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"
#include "bat/ads/internal/resources/parsing_result.h"
#include "bat/ads/internal/resources/resource_manager_observer.h"

namespace ads::resource {

class AntiTargeting final : public LocaleManagerObserver,
                            public ResourceManagerObserver {
 public:
  AntiTargeting();

  AntiTargeting(const AntiTargeting& other) = delete;
  AntiTargeting& operator=(const AntiTargeting& other) = delete;

  AntiTargeting(AntiTargeting&& other) noexcept = delete;
  AntiTargeting& operator=(AntiTargeting&& other) noexcept = delete;

  ~AntiTargeting() override;

  bool IsInitialized() const;

  void Load();

  const AntiTargetingInfo& get() const { return *anti_targeting_; }

 private:
  void OnLoadAndParseResource(ParsingResultPtr<AntiTargetingInfo> result);

  // LocaleManagerObserver:
  void OnLocaleDidChange(const std::string& locale) override;

  // ResourceManagerObserver:
  void OnResourceDidUpdate(const std::string& id) override;

  bool is_initialized_ = false;

  std::unique_ptr<AntiTargetingInfo> anti_targeting_;

  base::WeakPtrFactory<AntiTargeting> weak_ptr_factory_{this};
};

}  // namespace ads::resource

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_
