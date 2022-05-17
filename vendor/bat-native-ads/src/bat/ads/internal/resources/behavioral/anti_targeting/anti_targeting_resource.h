/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_info.h"
#include "bat/ads/internal/resources/parsing_result.h"
#include "bat/ads/internal/resources/resource_interface.h"

namespace ads {
namespace resource {

class AntiTargeting final : public ResourceInterface<AntiTargetingInfo> {
 public:
  AntiTargeting();
  ~AntiTargeting() override;

  AntiTargeting(const AntiTargeting&) = delete;
  AntiTargeting& operator=(const AntiTargeting&) = delete;

  bool IsInitialized() const override;

  void Load();

  AntiTargetingInfo get() const override;

 private:
  void OnLoadAndParseResource(ParsingResultPtr<AntiTargetingInfo> result);

  bool is_initialized_ = false;

  std::unique_ptr<AntiTargetingInfo> anti_targeting_;

  base::WeakPtrFactory<AntiTargeting> weak_ptr_factory_{this};
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_ANTI_TARGETING_ANTI_TARGETING_RESOURCE_H_
