/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_H_

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_resource.h"
#include "brave/components/brave_ads/core/internal/tabs/extracted_order_field_type.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/attempted_ad_event_key.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

class GURL;

namespace brave_ads {

class ConversionsObserver;
class TabManager;
struct OrderIdExtractionPatternsInfo;
struct OrderValueExtractionPatternsInfo;

class Conversions final : public TabManagerObserver {
 public:
  Conversions();

  Conversions(const Conversions&) = delete;
  Conversions& operator=(const Conversions&) = delete;

  ~Conversions() override;

  void AddObserver(ConversionsObserver* observer);
  void RemoveObserver(ConversionsObserver* observer);

  // Examine potential view-through or click-through conversions by matching URL
  // patterns in the redirect chain; an ad event converts at most once per
  // tab_id until that tab navigates away from the matching URL or closes.
  void MaybeConvert(int32_t tab_id, const std::vector<GURL>& redirect_chain);

  // `should_extract_id`/`should_extract_value` are the campaign's own
  // request; the patterns are always populated, for use only when true.
  using MaybeGetOrderPatternsCallback = base::OnceCallback<void(
      bool should_extract_id,
      OrderIdExtractionPatternsInfo id_extraction_patterns,
      bool should_extract_value,
      OrderValueExtractionPatternsInfo value_extraction_patterns)>;

  // `detected_language` comes from Translate's page detection, supplied by
  // the caller; with no value, tries every language in schema order.
  void MaybeGetOrderPatterns(int32_t tab_id,
                             const std::vector<GURL>& redirect_chain,
                             std::optional<std::string> detected_language,
                             MaybeGetOrderPatternsCallback callback);

  const OrderExtractionPatternsResource& GetOrderExtractionPatternsResource()
      const {
    return order_extraction_patterns_resource_;
  }

 private:
  void GetCampaignConversions(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain);
  void GetCampaignConversionsCallback(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      bool success,
      const CampaignConversionList& campaign_conversions);

  void GetAdEvents(int32_t tab_id,
                   const std::vector<GURL>& redirect_chain,
                   const CampaignConversionList& campaign_conversions);
  void GetAdEventsCallback(int32_t tab_id,
                           const std::vector<GURL>& redirect_chain,
                           const CampaignConversionList& campaign_conversions,
                           bool success,
                           const AdEventList& ad_events);

  void CheckForConversions(int32_t tab_id,
                           const std::vector<GURL>& redirect_chain,
                           const CampaignConversionList& campaign_conversions,
                           const AdEventList& ad_events);

  void MaybeGetOrderPatternsForCampaignConversionsCallback(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      std::optional<std::string> detected_language,
      MaybeGetOrderPatternsCallback callback,
      bool success,
      const CampaignConversionList& campaign_conversions);
  void MaybeGetOrderPatternsForAdEventsCallback(
      int32_t tab_id,
      const std::vector<GURL>& redirect_chain,
      std::optional<std::string> detected_language,
      MaybeGetOrderPatternsCallback callback,
      const CampaignConversionList& campaign_conversions,
      bool success,
      const AdEventList& ad_events);

  // Remembers that this instance requested order id/value extraction for
  // `tab_id`, so a conversion for that tab can wait for it to resolve.
  void MarkOrderExtractionPending(int32_t tab_id,
                                  bool should_extract_id,
                                  bool should_extract_value);

  // Whether `tab_id` has an order id or value extraction still in flight.
  bool IsOrderExtractionPendingForTab(int32_t tab_id) const;

  // Whether a newly-qualifying ad event for `tab_id` must wait rather than
  // convert now, because `campaign_conversion_bucket` requests an order id
  // or value that hasn't resolved yet. Marks `tab_id` as awaiting it, so a
  // conversion can never race ahead of that request becoming known.
  bool ShouldDeferConversionForOrderExtraction(
      int32_t tab_id,
      const CampaignConversionList& campaign_conversion_bucket);

  // Notifies any conversion deferred for `tab_id`, now that its order
  // extraction has fully resolved.
  void FlushDeferredConversionsForTab(int32_t tab_id);

  // Prunes attempted ad events for campaigns the tab no longer matches, so
  // a later, distinct purchase under the same ad event can convert again.
  void PruneAttemptedAdEventKeysForTab(
      int32_t tab_id,
      const CampaignConversionBucketMap& campaign_conversion_buckets);

  base::flat_set<AttemptedAdEventKey>* FindAttemptedAdEventKeys(
      int32_t tab_id,
      const std::string& campaign_id);

  void Convert(int32_t tab_id,
               const AdEventInfo& ad_event,
               const AttemptedAdEventKey& ad_event_key);
  void ConvertCallback(int32_t tab_id,
                       const AdEventInfo& ad_event,
                       const AttemptedAdEventKey& ad_event_key,
                       bool success);

  void NotifyDidConvertAd(const ConversionInfo& conversion);
  void NotifyFailedToConvertAd(const std::string& creative_instance_id);

  // TabManagerObserver:
  void OnDidOpenNewTab(const TabInfo& tab) override;
  void OnTabDidChange(const TabInfo& tab) override;
  void OnTabExtractedOrder(
      const TabInfo& tab,
      ExtractedOrderFieldType extracted_order_field_type) override;
  void OnDidCloseTab(int32_t tab_id) override;

  base::ObserverList<ConversionsObserver> observers_;

  // Per tab and campaign, ad events already converted since the tab last
  // matched the conversion URL pattern, so repeat matches cannot reconvert.
  base::flat_map<
      /*tab_id*/ int32_t,
      base::flat_map</*campaign_id*/ std::string,
                     base::flat_set<AttemptedAdEventKey>>>
      attempted_ad_event_keys_by_tab_id_;

  // Tabs with an order id/value extraction request that has not yet resolved
  // or timed out.
  base::flat_set<int32_t> tab_ids_awaiting_extracted_order_id_;
  base::flat_set<int32_t> tab_ids_awaiting_extracted_order_value_;

  // An ad event eligible to convert, held back only because its tab's order
  // extraction was still pending when it was matched.
  base::flat_map</*tab_id*/ int32_t,
                 std::vector<std::pair<AdEventInfo, AttemptedAdEventKey>>>
      deferred_ad_events_by_tab_id_;

  const database::table::CampaignConversions
      campaign_conversions_database_table_;

  const database::table::AdEvents ad_events_database_table_;

  const OrderExtractionPatternsResource order_extraction_patterns_resource_;

  base::ScopedObservation<TabManager, TabManagerObserver>
      tab_manager_observation_{this};

  base::WeakPtrFactory<Conversions> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_H_
