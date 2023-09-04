/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace brave_ads {

class PurchaseIntentResource;
struct PurchaseIntentSignalInfo;
struct PurchaseIntentSiteInfo;

class PurchaseIntentProcessor final : public TabManagerObserver {
 public:
  explicit PurchaseIntentProcessor(PurchaseIntentResource& resource);

  PurchaseIntentProcessor(const PurchaseIntentProcessor&) = delete;
  PurchaseIntentProcessor& operator=(const PurchaseIntentProcessor&) = delete;
  PurchaseIntentProcessor(PurchaseIntentProcessor&&) noexcept = delete;
  PurchaseIntentProcessor& operator=(PurchaseIntentProcessor&&) noexcept =
      delete;

  ~PurchaseIntentProcessor() override;

  void Process(const GURL& url);

 private:
  absl::optional<PurchaseIntentSignalInfo> ExtractSignal(const GURL& url) const;

  absl::optional<PurchaseIntentSiteInfo> GetSite(const GURL& url) const;

  absl::optional<SegmentList> GetSegmentsForSearchQuery(
      const std::string& search_query) const;

  uint16_t GetFunnelWeightForSearchQuery(const std::string& search_query) const;

  // TabManagerObserver:
  void OnTextContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& text) override;

  const raw_ref<PurchaseIntentResource> resource_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_PURCHASE_INTENT_PURCHASE_INTENT_PROCESSOR_H_
