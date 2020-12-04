/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_  // NOLINT

#include <string>
#include <vector>

#include "url/gurl.h"
#include "bat/ads/internal/ad_targeting/processors/processor.h"
#include "bat/ads/internal/ad_targeting/processors/purchase_intent/purchase_intent_signal_info.h"
#include "bat/ads/internal/ad_targeting/resources/purchase_intent/purchase_intent_resource.h"

namespace ads {
namespace ad_targeting {
namespace processor {

class PurchaseIntent : public Processor<GURL> {
 public:
  PurchaseIntent(
      resource::PurchaseIntent* resource);

  ~PurchaseIntent() override;

  void Process(
      const GURL& url) override;

 private:
  resource::PurchaseIntent* resource_;  // NOT OWNED

  PurchaseIntentSignalInfo ExtractIntentSignal(
      const std::string& url);

  void AppendIntentSignalToHistory(
      const PurchaseIntentSignalInfo& purchase_intent_signal);

  resource::SiteInfo GetSite(
      const std::string& url);

  PurchaseIntentSegmentList GetSegments(
      const std::string& search_query);

  uint16_t GetFunnelWeight(
      const std::string& search_query);

  bool IsSubset(
      const std::vector<std::string>& keyword_set_a,
      const std::vector<std::string>& keyword_set_b);

  std::vector<std::string> TransformIntoSetOfWords(
      const std::string& text);
};

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_  // NOLINT
