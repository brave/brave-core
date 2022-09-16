/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "bat/ads/internal/base/timer/timer.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversions_observer.h"
#include "bat/ads/internal/locale/locale_manager_observer.h"
#include "bat/ads/internal/resources/behavioral/conversions/conversion_id_pattern_info.h"
#include "bat/ads/internal/resources/resource_manager_observer.h"
#include "bat/ads/internal/tabs/tab_manager_observer.h"

class GURL;

namespace ads {

namespace resource {
class Conversions;
}  // namespace resource

struct AdEventInfo;
struct ConversionQueueItemInfo;
struct VerifiableConversionInfo;

class Conversions final : public LocaleManagerObserver,
                          public ResourceManagerObserver,
                          public TabManagerObserver {
 public:
  Conversions();

  Conversions(const Conversions&) = delete;
  Conversions& operator=(const Conversions&) = delete;

  Conversions(Conversions&& other) noexcept = delete;
  Conversions& operator=(Conversions&& other) noexcept = delete;

  ~Conversions() override;

  void AddObserver(ConversionsObserver* observer);
  void RemoveObserver(ConversionsObserver* observer);

  bool ShouldAllow() const;

  void MaybeConvert(const std::vector<GURL>& redirect_chain,
                    const std::string& html,
                    const ConversionIdPatternMap& conversion_id_patterns);

  void Process();

 private:
  void CheckRedirectChain(const std::vector<GURL>& redirect_chain,
                          const std::string& html,
                          const ConversionIdPatternMap& conversion_id_patterns);

  void Convert(const AdEventInfo& ad_event,
               const VerifiableConversionInfo& verifiable_conversion);

  ConversionList FilterConversions(const std::vector<GURL>& redirect_chain,
                                   const ConversionList& conversions);
  ConversionList SortConversions(const ConversionList& conversions);

  void AddItemToQueue(const AdEventInfo& ad_event,
                      const VerifiableConversionInfo& verifiable_conversion);
  void OnSaveConversionQueue(bool success);

  void ProcessQueueItem(const ConversionQueueItemInfo& queue_item);
  void ProcessQueue();

  void RemoveInvalidQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item);
  void OnRemoveInvalidQueueItem(
      const ConversionQueueItemInfo& conversion_queue_item,
      bool success);
  void MarkQueueItemAsProcessed(
      const ConversionQueueItemInfo& conversion_queue_item);
  void OnMarkQueueItemAsProcessed(
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

  // LocaleManagerObserver:
  void OnLocaleDidChange(const std::string& locale) override;

  // ResourceManagerObserver:
  void OnResourceDidUpdate(const std::string& id) override;

  // TabManagerObserver:
  void OnHtmlContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& content) override;

  base::ObserverList<ConversionsObserver> observers_;

  std::unique_ptr<resource::Conversions> resource_;

  Timer timer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_
