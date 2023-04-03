/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_result.h"

namespace brave_ads::resource {

struct ConversionsInfo;

class Conversions final {
 public:
  Conversions();

  Conversions(const Conversions& other) = delete;
  Conversions& operator=(const Conversions& other) = delete;

  Conversions(Conversions&& other) noexcept = delete;
  Conversions& operator=(Conversions&& other) noexcept = delete;

  ~Conversions();

  bool IsInitialized() const;

  void Load();

  const ConversionsInfo* get() const { return conversions_info_.get(); }

 private:
  void OnLoadAndParseResource(ParsingResultPtr<ConversionsInfo> result);

  bool is_initialized_ = false;

  std::unique_ptr<ConversionsInfo> conversions_info_;

  base::WeakPtrFactory<Conversions> weak_factory_{this};
};

}  // namespace brave_ads::resource

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_CONVERSIONS_CONVERSIONS_RESOURCE_H_
