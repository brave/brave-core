/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource.h"

class GURL;

namespace brave_ads {

class ConversionsObserver;
struct ConversionInfo;
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

  // Examine potential view-through or click-through conversions through various
  // channels, such as URL redirects or HTML pages.
  void MaybeConvert(const std::vector<GURL>& redirect_chain,
                    const std::string& html);

 private:
  void GetCreativeSetConversions(const std::vector<GURL>& redirect_chain,
                                 const std::string& html);
  void GetCreativeSetConversionsCallback(
      const std::vector<GURL>& redirect_chain,
      const std::string& html,
      bool success,
      const CreativeSetConversionList& creative_set_conversions);

  void GetAdEvents(const std::vector<GURL>& redirect_chain,
                   const std::string& html,
                   const CreativeSetConversionList& creative_set_conversions);
  void GetAdEventsCallback(
      const std::vector<GURL>& redirect_chain,
      const std::string& html,
      const CreativeSetConversionList& creative_set_conversions,
      bool success,
      const AdEventList& ad_events);

  void CheckForConversions(
      const std::vector<GURL>& redirect_chain,
      const std::string& html,
      const CreativeSetConversionList& creative_set_conversions,
      const AdEventList& ad_events);
  void Convert(const AdEventInfo& ad_event,
               std::optional<VerifiableConversionInfo> verifiable_conversion);
  void ConvertCallback(
      const AdEventInfo& ad_event,
      std::optional<VerifiableConversionInfo> verifiable_conversion,
      bool success);

  void NotifyDidConvertAd(const ConversionInfo& conversion) const;
  void NotifyFailedToConvertAd(const std::string& creative_instance_id) const;

  // TabManagerObserver:
  void OnHtmlContentDidChange(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& html) override;

  base::ObserverList<ConversionsObserver> observers_;

  ConversionResource resource_;

  const database::table::CreativeSetConversions
      creative_set_conversions_database_table_;

  const database::table::AdEvents ad_events_database_table_;

  base::WeakPtrFactory<Conversions> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_H_
