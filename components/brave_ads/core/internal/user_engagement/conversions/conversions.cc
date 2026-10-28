/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"

#include <ranges>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "url/gurl.h"

namespace brave_ads {

Conversions::Conversions() {
  tab_manager_observation_.Observe(&TabManager::GetInstance());
}

Conversions::~Conversions() = default;

void Conversions::AddObserver(ConversionsObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void Conversions::RemoveObserver(ConversionsObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

void Conversions::MaybeConvert(const std::vector<GURL>& redirect_chain) {
  CHECK(!redirect_chain.empty());

  BLOG(1, "Checking for campaign conversions");

  GetCampaignConversions(redirect_chain);
}

///////////////////////////////////////////////////////////////////////////////

void Conversions::GetCampaignConversions(
    const std::vector<GURL>& redirect_chain) {
  campaign_conversions_database_table_.GetUnexpired(
      base::BindOnce(&Conversions::GetCampaignConversionsCallback,
                     weak_factory_.GetWeakPtr(), redirect_chain));
}

void Conversions::GetCampaignConversionsCallback(
    const std::vector<GURL>& redirect_chain,
    bool success,
    const CampaignConversionList& campaign_conversions) {
  if (!success) {
    return BLOG(0, "Failed to get campaign conversions");
  }

  if (campaign_conversions.empty()) {
    return BLOG(1, "There are no campaign conversions");
  }

  GetAdEvents(redirect_chain, campaign_conversions);
}

void Conversions::GetAdEvents(
    const std::vector<GURL>& redirect_chain,
    const CampaignConversionList& campaign_conversions) {
  ad_events_database_table_.GetUnexpired(
      kAdEventsTimeWindow.Get(),
      base::BindOnce(&Conversions::GetAdEventsCallback,
                     weak_factory_.GetWeakPtr(), redirect_chain,
                     campaign_conversions));
}

void Conversions::GetAdEventsCallback(
    const std::vector<GURL>& redirect_chain,
    const CampaignConversionList& campaign_conversions,
    bool success,
    const AdEventList& ad_events) {
  if (!success) {
    return BLOG(0, "Failed to get ad events");
  }

  CheckForConversions(redirect_chain, campaign_conversions, ad_events);
}

void Conversions::CheckForConversions(
    const std::vector<GURL>& redirect_chain,
    const CampaignConversionList& campaign_conversions,
    const AdEventList& ad_events) {
  TRACE_EVENT(kTraceEventCategory, "Conversions::CheckForConversions",
              "redirect_chain", redirect_chain.size(), "campaign_conversions",
              campaign_conversions.size(), "ad_events", ad_events.size());

  const CampaignConversionList matching_campaign_conversions =
      GetMatchingCampaignConversions(campaign_conversions, redirect_chain);
  if (matching_campaign_conversions.empty()) {
    return BLOG(1, "There are no matching campaign conversions");
  }

  CampaignConversionCountMap campaign_conversion_counts =
      GetCampaignConversionCounts(ad_events);

  const size_t campaign_conversion_cap = kCampaignConversionCap.Get();

  CampaignConversionBucketMap campaign_conversion_buckets =
      SortCampaignConversionsIntoBuckets(matching_campaign_conversions);

  FilterCampaignConversionBucketsThatExceedTheCap(campaign_conversion_counts,
                                                  campaign_conversion_cap,
                                                  campaign_conversion_buckets);

  BLOG(1, matching_campaign_conversions.size()
              << " out of " << campaign_conversions.size()
              << " matching campaign conversions are sorted into "
              << campaign_conversion_buckets.size() << " buckets");

  // Click-through conversions should take priority over view-through
  // conversions. Ad events are ordered in chronological order by `created_at`;
  // click events are guaranteed to occur after view impression events.
  // Conversions are based on the last touch attribution model.
  bool did_convert = false;

  for (const auto& ad_event : std::views::reverse(ad_events)) {
    // Do we have campaign conversions for this ad event?
    const auto iter = campaign_conversion_buckets.find(ad_event.campaign_id);
    if (iter == campaign_conversion_buckets.cend()) {
      // No, so skip this ad event.
      continue;
    }
    const auto& [campaign_id, campaign_conversion_bucket] = *iter;

    // Have we exceeded the limit for campaign conversions?
    if (campaign_conversion_cap > 0 &&
        campaign_conversion_counts[campaign_id] == campaign_conversion_cap) {
      continue;
    }

    // Yes, so are we allowed to convert this ad event?
    if (!IsAllowedToConvertAdEvent(ad_event)) {
      // No, so skip this ad event.
      continue;
    }

    // Yes, so convert the ad event if it occurs within the observation
    // window of any matching campaign conversion.
    if (!HasCampaignConversionWithinObservationWindow(
            campaign_conversion_bucket, ad_event)) {
      continue;
    }

    Convert(ad_event);

    did_convert = true;

    // Have we exceeded the limit for campaign conversions?
    ++campaign_conversion_counts[campaign_id];

    if (did_convert) {
      // Remove the bucket for this campaign so that we deduplicate
      // conversions for the remainder of the ad events.
      campaign_conversion_buckets.erase(campaign_id);
    }
  }

  if (!did_convert) {
    BLOG(1, "There were no conversion matches");
  }
}

void Conversions::Convert(const AdEventInfo& ad_event) {
  RecordAdEvent(RebuildAdEvent(ad_event, mojom::ConfirmationType::kConversion,
                               /*created_at=*/base::Time::Now()),
                base::BindOnce(&Conversions::ConvertCallback,
                               weak_factory_.GetWeakPtr(), ad_event));
}

void Conversions::ConvertCallback(const AdEventInfo& ad_event, bool success) {
  if (!success) {
    BLOG(0, "Failed to record ad conversion event");
    return NotifyFailedToConvertAd(ad_event.creative_instance_id);
  }

  const ConversionInfo conversion = BuildConversion(ad_event);
  NotifyDidConvertAd(conversion);
}

void Conversions::NotifyDidConvertAd(const ConversionInfo& conversion) {
  observers_.Notify(&ConversionsObserver::OnDidConvertAd, conversion);
}

void Conversions::NotifyFailedToConvertAd(
    const std::string& creative_instance_id) {
  observers_.Notify(&ConversionsObserver::OnFailedToConvertAd,
                    creative_instance_id);
}

void Conversions::OnTabDidChange(const TabInfo& tab) {
  MaybeConvert(tab.redirect_chain);
}

}  // namespace brave_ads
