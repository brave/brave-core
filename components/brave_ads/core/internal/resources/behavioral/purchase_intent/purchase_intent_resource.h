/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_error_or.h"

namespace brave_ads::resource {

class PurchaseIntent final {
 public:
  PurchaseIntent();

  PurchaseIntent(const PurchaseIntent&) = delete;
  PurchaseIntent& operator=(const PurchaseIntent&) = delete;

  PurchaseIntent(PurchaseIntent&&) noexcept = delete;
  PurchaseIntent& operator=(PurchaseIntent&&) noexcept = delete;

  ~PurchaseIntent();

  bool IsInitialized() const { return is_initialized_; }

  void Load();

  const targeting::PurchaseIntentInfo* Get() const;

 private:
  void OnLoadAndParseResource(
      ParsingErrorOr<targeting::PurchaseIntentInfo> result);

  bool is_initialized_ = false;

  targeting::PurchaseIntentInfo purchase_intent_;

  base::WeakPtrFactory<PurchaseIntent> weak_factory_{this};
};

}  // namespace brave_ads::resource

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
