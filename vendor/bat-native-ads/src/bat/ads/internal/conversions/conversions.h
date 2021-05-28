/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/conversions_observer.h"
#include "bat/ads/internal/conversions/verifiable_conversion_info.h"
#include "bat/ads/internal/resources/conversions/conversion_id_pattern_info.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class Conversions {
 public:
  Conversions();

  ~Conversions();

  void AddObserver(ConversionsObserver* observer);
  void RemoveObserver(ConversionsObserver* observer);

  bool ShouldAllow() const;

  void MaybeConvert(const std::vector<std::string>& redirect_chain,
                    const std::string& html,
                    const ConversionIdPatternMap& conversion_id_patterns);

  void StartTimerIfReady();

 private:
  base::ObserverList<ConversionsObserver> observers_;

  Timer timer_;

  void CheckRedirectChain(const std::vector<std::string>& redirect_chain,
                          const std::string& html,
                          const ConversionIdPatternMap& conversion_id_patterns);

  void Convert(const AdEventInfo& ad_event,
               const VerifiableConversionInfo& verifiable_conversion);

  ConversionList FilterConversions(
      const std::vector<std::string>& redirect_chain,
      const ConversionList& conversions);
  ConversionList SortConversions(const ConversionList& conversions);

  void AddItemToQueue(const AdEventInfo& ad_event,
                      const VerifiableConversionInfo& verifiable_conversion);

  bool RemoveItemFromQueue(
      const ConversionQueueItemInfo& conversion_queue_item);
  void ProcessQueueItem(const ConversionQueueItemInfo& queue_item);
  void ProcessQueue();

  void StartTimer(const ConversionQueueItemInfo& queue_item);

  void NotifyConversion(
      const ConversionQueueItemInfo& conversion_queue_item) const;
  void NotifyConversionFailed(
      const ConversionQueueItemInfo& conversion_queue_item) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_H_
