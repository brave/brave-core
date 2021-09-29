/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_CONVERSIONS_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_CONVERSIONS_RESOURCE_H_

#include <string>

#include "bat/ads/internal/resources/conversions/conversion_id_pattern_info_aliases.h"
#include "bat/ads/internal/resources/resource.h"

namespace ads {
namespace resource {

class Conversions final : public Resource<ConversionIdPatternMap> {
 public:
  Conversions();
  ~Conversions() override;

  Conversions(const Conversions&) = delete;
  Conversions& operator=(const Conversions&) = delete;

  bool IsInitialized() const override;

  void Load();

  ConversionIdPatternMap get() const override;

 private:
  bool is_initialized_ = false;

  ConversionIdPatternMap conversion_id_patterns_;

  bool FromJson(const std::string& json);
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_CONVERSIONS_RESOURCE_H_
