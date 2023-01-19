/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/locale/locale_manager_observer.h"
#include "bat/ads/internal/resources/resource_manager_observer.h"
#include "bat/ads/internal/segments/segment_alias.h"
#include "bat/ads/internal/tabs/tab_manager_observer.h"

class GURL;

namespace ads {

namespace resource {
class PurchaseIntent;
}  // namespace resource

namespace targeting {
struct PurchaseIntentSignalInfo;
struct PurchaseIntentSiteInfo;
}  // namespace targeting

namespace processor {

class PurchaseIntent final : public LocaleManagerObserver,
                             public ResourceManagerObserver,
                             public TabManagerObserver {
 public:
  explicit PurchaseIntent(resource::PurchaseIntent* resource);

  PurchaseIntent(const PurchaseIntent& other) = delete;
  PurchaseIntent& operator=(const PurchaseIntent& other) = delete;

  PurchaseIntent(PurchaseIntent&& other) noexcept = delete;
  PurchaseIntent& operator=(PurchaseIntent&& other) noexcept = delete;

  ~PurchaseIntent() override;

  void Process(const GURL& url);

 private:
  targeting::PurchaseIntentSignalInfo ExtractSignal(const GURL& url) const;

  targeting::PurchaseIntentSiteInfo GetSite(const GURL& url) const;

  SegmentList GetSegmentsForSearchQuery(const std::string& search_query) const;

  uint16_t GetFunnelWeightForSearchQuery(const std::string& search_query) const;

  // LocaleManagerObserver:
  void OnLocaleDidChange(const std::string& locale) override;

  // ResourceManagerObserver:
  void OnResourceDidUpdate(const std::string& id) override;

  // TabManagerObserver:
  void OnTextContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& content) override;

  const raw_ptr<resource::PurchaseIntent> resource_ = nullptr;  // NOT OWNED
};

}  // namespace processor
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_
