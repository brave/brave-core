/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_RESOURCE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/resources/parsing_result.h"

namespace ads {

namespace targeting {
struct PurchaseIntentInfo;
}  // namespace targeting

namespace resource {

class PurchaseIntent final {
 public:
  PurchaseIntent();

  PurchaseIntent(const PurchaseIntent& other) = delete;
  PurchaseIntent& operator=(const PurchaseIntent& other) = delete;

  PurchaseIntent(PurchaseIntent&& other) noexcept = delete;
  PurchaseIntent& operator=(PurchaseIntent&& other) noexcept = delete;

  ~PurchaseIntent();

  bool IsInitialized() const;

  void Load();

  const targeting::PurchaseIntentInfo* Get() const;

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
