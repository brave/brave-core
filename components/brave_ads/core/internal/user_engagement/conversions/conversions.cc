/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"

#include "base/check.h"
#include "base/containers/adapters.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

Conversions::Conversions() {
  TabManager::GetInstance().AddObserver(this);
}

Conversions::~Conversions() {
  TabManager::GetInstance().RemoveObserver(this);
}

void Conversions::AddObserver(ConversionsObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void Conversions::RemoveObserver(ConversionsObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

void Conversions::MaybeConvert(const std::vector<GURL>& redirect_chain,
                               const std::string& html) {
  CHECK(!redirect_chain.empty());

  BLOG(1, "Checking for creative set conversions");

  GetCreativeSetConversions(redirect_chain, html);
}

///////////////////////////////////////////////////////////////////////////////

void Conversions::GetCreativeSetConversions(
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  creative_set_conversions_database_table_.GetUnexpired(
      base::BindOnce(&Conversions::GetCreativeSetConversionsCallback,
                     weak_factory_.GetWeakPtr(), redirect_chain, html));
}

void Conversions::GetCreativeSetConversionsCallback(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const bool success,
    const CreativeSetConversionList& creative_set_conversions) {
  if (!success) {
    return BLOG(0, "Failed to get creative set conversions");
  }

  if (creative_set_conversions.empty()) {
    return BLOG(1, "There are no creative set conversions");
  }

  GetAdEvents(redirect_chain, html, creative_set_conversions);
}

void Conversions::GetAdEvents(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const CreativeSetConversionList& creative_set_conversions) {
  ad_events_database_table_.GetUnexpired(base::BindOnce(
      &Conversions::GetAdEventsCallback, weak_factory_.GetWeakPtr(),
      redirect_chain, html, creative_set_conversions));
}

void Conversions::GetAdEventsCallback(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const CreativeSetConversionList& creative_set_conversions,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    return BLOG(0, "Failed to get ad events");
  }

  CheckForConversions(redirect_chain, html, creative_set_conversions,
                      ad_events);
}

void Conversions::CheckForConversions(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const CreativeSetConversionList& creative_set_conversions,
    const AdEventList& ad_events) {
  const CreativeSetConversionList matching_creative_set_conversions =
      GetMatchingCreativeSetConversions(creative_set_conversions,
                                        redirect_chain);
  if (matching_creative_set_conversions.empty()) {
    return BLOG(1, "There are no matching creative set conversions");
  }

  CreativeSetConversionCountMap creative_set_conversion_counts =
      GetCreativeSetConversionCounts(ad_events);

  const size_t creative_set_conversion_cap = kCreativeSetConversionCap.Get();

  CreativeSetConversionBucketMap creative_set_conversion_buckets =
      SortCreativeSetConversionsIntoBuckets(matching_creative_set_conversions);

  FilterCreativeSetConversionBucketsThatExceedTheCap(
      creative_set_conversion_counts, creative_set_conversion_cap,
      creative_set_conversion_buckets);

  BLOG(1, matching_creative_set_conversions.size()
              << " out of " << creative_set_conversions.size()
              << " matching creative set conversions are sorted into "
              << creative_set_conversion_buckets.size() << " buckets");

  // Click-through conversions should take priority over view-through
  // conversions. Ad events are ordered in chronological order by `created_at`;
  // click events are guaranteed to occur after view impression events.
  // Conversions are based on the last touch attribution model.
  bool did_convert = false;

  for (const auto& ad_event : base::Reversed(ad_events)) {
    // Do we have creative set conversions for this ad event?
    const auto iter =
        creative_set_conversion_buckets.find(ad_event.creative_set_id);
    if (iter == creative_set_conversion_buckets.cend()) {
      // No, so skip this ad event.
      continue;
    }
    const auto& [creative_set_id, creative_set_conversion_bucket] = *iter;

    // Have we exceeded the limit for creative set conversions?
    if (creative_set_conversion_cap > 0 &&
        creative_set_conversion_counts[creative_set_id] ==
            creative_set_conversion_cap) {
      continue;
    }

    // Yes, so are we allowed to convert this ad event?
    if (!IsAllowedToConvertAdEvent(ad_event)) {
      // No, so skip this ad event.
      continue;
    }

    // Yes, so convert the ad event where it occurs within the observation
    // window for the set of creative conversions.
    for (const auto& creative_set_conversion :
         GetCreativeSetConversionsWithinObservationWindow(
             creative_set_conversion_bucket, ad_event)) {
      std::optional<VerifiableConversionInfo> verifiable_conversion;
      if (const std::optional<ConversionResourceInfo>& conversion_resource =
              resource_.get()) {
        // Attempt to build a verifiable conversion only if the conversion
        // resource is available.
        verifiable_conversion = MaybeBuildVerifiableConversion(
            redirect_chain, html, conversion_resource->id_patterns,
            creative_set_conversion);
      }

      Convert(ad_event, verifiable_conversion);

      did_convert = true;

      // Have we exceeded the limit for creative set conversions?
      ++creative_set_conversion_counts[creative_set_id];
      if (creative_set_conversion_counts[creative_set_id] ==
          creative_set_conversion_cap) {
        // Yes, so stop converting.
        break;
      }
    }

    if (did_convert) {
      // Remove the bucket for this creative set so that we deduplicate
      // conversions for the remainder of the ad events.
      creative_set_conversion_buckets.erase(creative_set_id);
    }
  }

  if (!did_convert) {
    BLOG(1, "There were no conversion matches");
  }
}

void Conversions::Convert(
    const AdEventInfo& ad_event,
    const std::optional<VerifiableConversionInfo>& verifiable_conversion) {
  RecordAdEvent(
      RebuildAdEvent(ad_event, mojom::ConfirmationType::kConversion,
                     /*created_at=*/base::Time::Now()),
      base::BindOnce(&Conversions::ConvertCallback, weak_factory_.GetWeakPtr(),
                     ad_event, verifiable_conversion));
}

void Conversions::ConvertCallback(
    const AdEventInfo& ad_event,
    const std::optional<VerifiableConversionInfo>& verifiable_conversion,
    const bool success) {
  if (!success) {
    BLOG(0, "Failed to record ad conversion event");

    return NotifyFailedToConvertAd(ad_event.creative_instance_id);
  }

  const ConversionInfo conversion =
      BuildConversion(ad_event, verifiable_conversion);
  NotifyDidConvertAd(conversion);
}

void Conversions::NotifyDidConvertAd(const ConversionInfo& conversion) const {
  for (ConversionsObserver& observer : observers_) {
    observer.OnDidConvertAd(conversion);
  }
}

void Conversions::NotifyFailedToConvertAd(
    const std::string& creative_instance_id) const {
  for (ConversionsObserver& observer : observers_) {
    observer.OnFailedToConvertAd(creative_instance_id);
  }
}

void Conversions::OnHtmlContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  MaybeConvert(redirect_chain, html);
}

}  // namespace brave_ads
