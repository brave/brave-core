/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/resources/parsing_result.h"

namespace brave_ads {

namespace targeting {
struct PurchaseIntentInfo;
}  // namespace targeting

namespace resource {

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
      ParsingResultPtr<targeting::PurchaseIntentInfo> result);

  bool is_initialized_ = false;

  std::unique_ptr<targeting::PurchaseIntentInfo> purchase_intent_;

  base::WeakPtrFactory<PurchaseIntent> weak_factory_{this};
};

}  // namespace resource
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
