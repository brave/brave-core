/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversion_id_pattern_info_aliases.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversions_info.h"
#include "bat/ads/internal/resources/parsing_result.h"
#include "bat/ads/internal/resources/resource_interface.h"

namespace ads {
namespace resource {

class Conversions final : public ResourceInterface<const ConversionsInfo*> {
 public:
  Conversions();
  ~Conversions() override;

  Conversions(const Conversions&) = delete;
  Conversions& operator=(const Conversions&) = delete;

  bool IsInitialized() const override;

  void Load();

  const ConversionsInfo* get() const override;

 private:
  void OnLoadAndParseResource(ParsingResultPtr<ConversionsInfo> result);

  bool is_initialized_ = false;

  std::unique_ptr<ConversionsInfo> conversions_info_;

  base::WeakPtrFactory<Conversions> weak_ptr_factory_{this};
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_
