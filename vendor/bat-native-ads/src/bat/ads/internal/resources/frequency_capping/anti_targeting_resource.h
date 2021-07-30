/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_RESOURCE_H_

#include <string>

#include "bat/ads/internal/resources/frequency_capping/anti_targeting_info.h"
#include "bat/ads/internal/resources/resource.h"

namespace ads {
namespace resource {

class AntiTargeting : public Resource<AntiTargetingInfo> {
 public:
  AntiTargeting();
  ~AntiTargeting() override;

  AntiTargeting(const AntiTargeting&) = delete;
  AntiTargeting& operator=(const AntiTargeting&) = delete;

  bool IsInitialized() const override;

  void Load();

  AntiTargetingInfo get() const override;

 private:
  bool is_initialized_ = false;

  AntiTargetingInfo anti_targeting_;

  bool FromJson(const std::string& json);
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_FREQUENCY_CAPPING_ANTI_TARGETING_RESOURCE_H_
