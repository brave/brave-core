/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_info.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversion_id_pattern_info.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversions_resource.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"

class GURL;

namespace brave_ads {

struct AdEventInfo;
struct VerifiableConversionInfo;

class Conversions final : public TabManagerObserver {
 public:
  Conversions();

  Conversions(const Conversions&) = delete;
  Conversions& operator=(const Conversions&) = delete;

  Conversions(Conversions&&) noexcept = delete;
  Conversions& operator=(Conversions&&) noexcept = delete;

  ~Conversions() override;

  void AddObserver(ConversionsObserver* observer);
  void RemoveObserver(ConversionsObserver* observer);

  void MaybeConvert(const std::vector<GURL>& redirect_chain,
                    const std::string& html,
                    const ConversionIdPatternMap& conversion_id_patterns);

  void Process();

 private:
  void GetUnprocessedConversionsCallback(
      bool success,
      const ConversionQueueItemList& conversion_queue_items);

  void CheckRedirectChain(const std::vector<GURL>& redirect_chain,
                          const std::string& html,
                          const ConversionIdPatternMap& conversion_id_patterns);
  void GetAllAdEventsCallback(std::vector<GURL> redirect_chain,
                              std::string html,
                              ConversionIdPatternMap conversion_id_patterns,
                              bool success,
                              const AdEventList& ad_events);
  void GetAllConversionsCallback(
      const std::vector<GURL>& redirect_chain,
      const std::string& html,
      const ConversionIdPatternMap& conversion_id_patterns,
      const AdEventList& ad_events,
      bool success,
      const ConversionList& conversions);

  void Convert(const AdEventInfo& ad_event,
               const VerifiableConversionInfo& verifiable_conversion);

  void AddItemToQueue(const AdEventInfo& ad_event,
                      const VerifiableConversionInfo& verifiable_conversion);
  void SaveConversionQueueCallback(bool success);

  void ProcessQueueItem(const ConversionQueueItemInfo& queue_item);
  void GetConversionQueueCallback(
      bool success,
      const ConversionQueueItemList& conversion_queue_items);
  void ProcessQueue();

  void RemoveInvalidQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item);
  void RemoveInvalidQueueItemCallback(
      const ConversionQueueItemInfo& conversion_queue_item,
      bool success);
  void MarkQueueItemAsProcessed(
      const ConversionQueueItemInfo& conversion_queue_item);
  void MarkQueueItemAsProcessedCallback(
      const ConversionQueueItemInfo& conversion_queue_item,
      bool success);
  void FailedToConvertQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item);
  void ConvertedQueueItem(const ConversionQueueItemInfo& conversion_queue_item);

  void StartTimer(const ConversionQueueItemInfo& queue_item);

  void NotifyConversion(
      const ConversionQueueItemInfo& conversion_queue_item) const;
  void NotifyConversionFailed(
      const ConversionQueueItemInfo& conversion_queue_item) const;

  // TabManagerObserver:
  void OnHtmlContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& content) override;

  base::ObserverList<ConversionsObserver> observers_;

  ConversionsResource resource_;

  Timer timer_;

  base::WeakPtrFactory<Conversions> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_H_
