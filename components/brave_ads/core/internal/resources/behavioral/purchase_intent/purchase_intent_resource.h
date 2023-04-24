/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_info.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_error_or.h"

namespace brave_ads {

class PurchaseIntentResource final {
 public:
  PurchaseIntentResource();

  PurchaseIntentResource(const PurchaseIntentResource&) = delete;
  PurchaseIntentResource& operator=(const PurchaseIntentResource&) = delete;

  PurchaseIntentResource(PurchaseIntentResource&&) noexcept = delete;
  PurchaseIntentResource& operator=(PurchaseIntentResource&&) noexcept = delete;

  ~PurchaseIntentResource();

  bool IsInitialized() const { return is_initialized_; }

  void Load();

  const PurchaseIntentInfo* Get() const;

 private:
  void OnLoadAndParseResource(
      ResourceParsingErrorOr<PurchaseIntentInfo> result);

  bool is_initialized_ = false;

  PurchaseIntentInfo purchase_intent_;

  base::WeakPtrFactory<PurchaseIntentResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
