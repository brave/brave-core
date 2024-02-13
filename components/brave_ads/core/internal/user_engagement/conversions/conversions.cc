/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "url/gurl.h"

namespace brave_ads {

Conversions::Conversions() {
  TabManager::GetInstance().AddObserver(this);
  queue_.SetDelegate(this);
}

Conversions::~Conversions() {
  TabManager::GetInstance().RemoveObserver(this);
}

void Conversions::AddObserver(ConversionsObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void Conversions::RemoveObserver(ConversionsObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

void Conversions::MaybeConvert(const std::vector<GURL>& redirect_chain,
                               const std::string& html) {
  if (redirect_chain.empty()) {
    return;
  }

  BLOG(1, "Checking for conversions");

  GetCreativeSetConversions(redirect_chain, html);
}

///////////////////////////////////////////////////////////////////////////////

void Conversions::GetCreativeSetConversions(
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  const database::table::CreativeSetConversions database_table;
  database_table.GetAll(
      base::BindOnce(&Conversions::GetCreativeSetConversionsCallback,
                     weak_factory_.GetWeakPtr(), redirect_chain, html));
}

void Conversions::GetCreativeSetConversionsCallback(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const bool success,
    const CreativeSetConversionList& creative_set_conversions) {
  if (!success) {
    return BLOG(1, "Failed to get creative set conversions");
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
  const database::table::AdEvents database_table;
  database_table.GetAll(base::BindOnce(
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
    return BLOG(1, "Failed to get ad events");
  }

  CheckForConversions(redirect_chain, html, creative_set_conversions,
                      ad_events);
}

void Conversions::CheckForConversions(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const CreativeSetConversionList& creative_set_conversions,
    const AdEventList& ad_events) {
  const CreativeSetConversionList filtered_creative_set_conversions =
      FilterConvertedAndNonMatchingCreativeSetConversions(
          creative_set_conversions, ad_events, redirect_chain);
  if (filtered_creative_set_conversions.empty()) {
    return BLOG(1, "There are no eligible creative set conversions");
  }

  CreativeSetConversionBucketMap creative_set_conversion_buckets =
      SortCreativeSetConversionsIntoBuckets(filtered_creative_set_conversions);

  BLOG(1, filtered_creative_set_conversions.size()
              << " out of " << creative_set_conversions.size()
              << " eligible creative set conversions are sorted into "
              << creative_set_conversion_buckets.size() << " buckets");

  bool did_convert = false;

  // Click-through conversions should take priority over view-through
  // conversions. Ad events are ordered in descending order by `created_at`;
  // click events are guaranteed to occur after view events.
  for (const auto& ad_event : ad_events) {
    // Do we have a bucket with creative set conversions for this ad event?
    const auto iter =
        creative_set_conversion_buckets.find(ad_event.creative_set_id);
    if (iter == creative_set_conversion_buckets.cend()) {
      // No, because the creative set has already been converted, or there are
      // no conversions for this creative set.
      continue;
    }

    if (!CanConvertAdEvent(ad_event)) {
      continue;
    }

    const auto& [_, creative_set_conversion_bucket] = *iter;
    const std::optional<CreativeSetConversionInfo> creative_set_conversion =
        FindNonExpiredCreativeSetConversion(creative_set_conversion_bucket,
                                            ad_event);
    if (!creative_set_conversion) {
      continue;
    }

    Convert(ad_event, MaybeBuildVerifiableConversion(
                          redirect_chain, html, resource_.get().id_patterns,
                          *creative_set_conversion));

    did_convert = true;

    // Remove the bucket for this creative set because we should not convert
    // another ad event from the same creative set.
    creative_set_conversion_buckets.erase(ad_event.creative_set_id);
    if (creative_set_conversion_buckets.empty()) {
      // All the buckets have drained, so they will no longer contain
      // conversions; therefore, we should consider our job done!
      break;
    }
  }

  if (!did_convert) {
    BLOG(1, "There are no matching conversions");
  }
}

void Conversions::Convert(
    const AdEventInfo& ad_event,
    const std::optional<VerifiableConversionInfo>& verifiable_conversion) {
  BLOG(1, "Conversion for " << ad_event.type << " with creative instance id "
                            << ad_event.creative_instance_id
                            << ", creative set id " << ad_event.creative_set_id
                            << ", campaign id " << ad_event.campaign_id
                            << " and advertiser id " << ad_event.advertiser_id);

  RecordAdEvent(
      RebuildAdEvent(ad_event, ConfirmationType::kConversion,
                     base::Time::Now()),
      base::BindOnce(&Conversions::ConvertCallback, weak_factory_.GetWeakPtr(),
                     ad_event, verifiable_conversion));
}

void Conversions::ConvertCallback(
    const AdEventInfo& ad_event,
    const std::optional<VerifiableConversionInfo>& verifiable_conversion,
    const bool success) {
  if (!success) {
    BLOG(1, "Failed to record conversion event");
    return NotifyFailedToConvertAd(ad_event.creative_instance_id);
  }

  const ConversionInfo conversion =
      BuildConversion(ad_event, verifiable_conversion);
  queue_.Add(conversion);
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

void Conversions::OnDidAddConversionToQueue(const ConversionInfo& conversion) {
  BLOG(1, "Successfully added "
              << ToString(conversion.action_type) << " "
              << ConversionTypeToString(conversion) << " for "
              << conversion.ad_type << " with creative instance id "
              << conversion.creative_instance_id << ", creative set id "
              << conversion.creative_set_id << ", campaign id "
              << conversion.campaign_id << " and advertiser id "
              << conversion.advertiser_id << " to the queue");
}

void Conversions::OnFailedToAddConversionToQueue(
    const ConversionInfo& conversion) {
  BLOG(1, "Failed to add " << ToString(conversion.action_type) << " "
                           << ConversionTypeToString(conversion) << " for "
                           << conversion.ad_type
                           << " with creative instance id "
                           << conversion.creative_instance_id
                           << ", creative set id " << conversion.creative_set_id
                           << ", campaign id " << conversion.campaign_id
                           << " and advertiser id " << conversion.advertiser_id
                           << " to the queue");
}

void Conversions::OnWillProcessConversionQueue(const ConversionInfo& conversion,
                                               const base::Time process_at) {
  BLOG(1, "Process " << ToString(conversion.action_type) << " "
                     << ConversionTypeToString(conversion) << " for "
                     << conversion.ad_type << " with creative instance id "
                     << conversion.creative_instance_id << ", creative set id "
                     << conversion.creative_set_id << ", campaign id "
                     << conversion.campaign_id << " and advertiser id "
                     << conversion.advertiser_id << " "
                     << FriendlyDateAndTime(process_at));
}

void Conversions::OnDidProcessConversionQueue(
    const ConversionInfo& conversion) {
  BLOG(1, "Successfully processed "
              << ToString(conversion.action_type) << " "
              << ConversionTypeToString(conversion) << " for "
              << conversion.ad_type << " with creative instance id "
              << conversion.creative_instance_id << ", creative set id "
              << conversion.creative_set_id << ", campaign id "
              << conversion.campaign_id << " and advertiser id "
              << conversion.advertiser_id);

  NotifyDidConvertAd(conversion);
}

void Conversions::OnFailedToProcessConversionQueue(
    const ConversionInfo& conversion) {
  BLOG(1, "Failed to process "
              << ToString(conversion.action_type) << " "
              << ConversionTypeToString(conversion) << " for "
              << conversion.ad_type << " with creative instance id "
              << conversion.creative_instance_id << ", creative set id "
              << conversion.creative_set_id << ", campaign id "
              << conversion.campaign_id << " and advertiser id "
              << conversion.advertiser_id);

  NotifyFailedToConvertAd(conversion.creative_instance_id);
}

void Conversions::OnDidExhaustConversionQueue() {
  BLOG(1, "Conversion queue is exhausted");
}

void Conversions::OnHtmlContentDidChange(
    const int32_t /*tab_id*/,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  MaybeConvert(redirect_chain, html);
}

}  // namespace brave_ads
