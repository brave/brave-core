/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_

#include <cstdint>
#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/segments/segments_aliases.h"
#include "bat/ads/internal/targeting/processors/processor_interface.h"
#include "url/gurl.h"

namespace ads {

namespace resource {
class PurchaseIntent;
}  // namespace resource

namespace targeting {

struct PurchaseIntentSignalInfo;
struct PurchaseIntentSiteInfo;

namespace processor {

class PurchaseIntent final : public ProcessorInterface<GURL> {
 public:
  explicit PurchaseIntent(resource::PurchaseIntent* resource);
  ~PurchaseIntent() override;

  void Process(const GURL& url) override;

 private:
  PurchaseIntentSignalInfo ExtractSignal(const GURL& url) const;

  PurchaseIntentSiteInfo GetSite(const GURL& url) const;

  SegmentList GetSegmentsForSearchQuery(const std::string& search_query) const;

  uint16_t GetFunnelWeightForSearchQuery(const std::string& search_query) const;

  raw_ptr<resource::PurchaseIntent> resource_ = nullptr;  // NOT OWNED
};

}  // namespace processor
}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TARGETING_PROCESSORS_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_
