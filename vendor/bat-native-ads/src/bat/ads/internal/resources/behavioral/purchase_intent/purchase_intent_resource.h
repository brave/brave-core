/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/resources/parsing_result.h"
#include "bat/ads/internal/resources/resource_interface.h"

namespace ads {

namespace targeting {
struct PurchaseIntentInfo;
}  // namespace targeting

namespace resource {

class PurchaseIntent final
    : public ResourceInterface<const targeting::PurchaseIntentInfo*> {
 public:
  PurchaseIntent();
  ~PurchaseIntent() override;

  PurchaseIntent(const PurchaseIntent&) = delete;
  PurchaseIntent& operator=(const PurchaseIntent&) = delete;

  bool IsInitialized() const override;

  void Load();

  const targeting::PurchaseIntentInfo* get() const override;

 private:
  void OnLoadAndParseResource(
      ParsingResultPtr<targeting::PurchaseIntentInfo> result);

  bool is_initialized_ = false;

  std::unique_ptr<targeting::PurchaseIntentInfo> purchase_intent_;

  base::WeakPtrFactory<PurchaseIntent> weak_ptr_factory_{this};
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
