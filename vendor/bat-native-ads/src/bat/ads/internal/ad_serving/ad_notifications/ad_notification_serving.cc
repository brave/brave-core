/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"

#include <cstdint>
#include <string>
#include <vector>

#include "base/guid.h"
#include "base/rand_util.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ad_delivery/ad_notifications/ad_notification_delivery.h"
#include "bat/ads/internal/ad_pacing/ad_notifications/ad_notification_pacing.h"
#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving_features.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/contextual/text_classification/text_classification_model.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_segment_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_values.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"
#include "bat/ads/internal/frequency_capping/ad_notifications/ad_notifications_frequency_capping.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/p2a/p2a.h"
#include "bat/ads/internal/p2a/p2a_util.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/time_formatting_util.h"

namespace ads {
namespace ad_notifications {

AdServing::AdServing(
    AdTargeting* ad_targeting,
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting)
    : ad_targeting_(ad_targeting),
      subdivision_targeting_(subdivision_targeting),
      anti_targeting_resource_(anti_targeting) {
  DCHECK(ad_targeting_);
  DCHECK(subdivision_targeting_);
  DCHECK(anti_targeting_resource_);
}

AdServing::~AdServing() = default;

void AdServing::ServeAtRegularIntervals() {
  if (timer_.IsRunning()) {
    return;
  }

  base::TimeDelta delay;

  if (Client::Get()->GetNextAdServingInterval().is_null()) {
    delay = base::TimeDelta::FromMinutes(2);
    const base::Time next_interval = base::Time::Now() + delay;

    Client::Get()->SetNextAdServingInterval(next_interval);
  } else {
    if (NextIntervalHasElapsed()) {
      delay = base::TimeDelta::FromMinutes(1);
    } else {
      const base::Time next_interval =
          Client::Get()->GetNextAdServingInterval();

      delay = next_interval - base::Time::Now();
    }
  }

  const base::Time next_interval = MaybeServeAfter(delay);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(next_interval));
}

void AdServing::StopServing() {
  timer_.Stop();
}

void AdServing::MaybeServe() {
  const SegmentList segments = ad_targeting_->GetSegments();

  MaybeServeAdForSegments(segments, [&](const Result result,
                                        const AdNotificationInfo& ad) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Ad notification not delivered");
      FailedToDeliverAd();
      return;
    }

    BLOG(1, "Ad notification delivered:\n"
                << "  uuid: " << ad.uuid << "\n"
                << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
                << "  creativeSetId: " << ad.creative_set_id << "\n"
                << "  campaignId: " << ad.campaign_id << "\n"
                << "  advertiserId: " << ad.advertiser_id << "\n"
                << "  segment: " << ad.segment << "\n"
                << "  title: " << ad.title << "\n"
                << "  body: " << ad.body << "\n"
                << "  targetUrl: " << ad.target_url);

    DeliveredAd();
  });
}

void AdServing::OnAdsPerHourChanged() {
  const int64_t ads_per_hour = settings::GetAdsPerHour();
  BLOG(1, "Maximum ads per hour changed to " << ads_per_hour);

  if (!PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  MaybeServeNextAd();
}

///////////////////////////////////////////////////////////////////////////////

void AdServing::MaybeServeNextAd() {
  const int64_t ads_per_hour = settings::GetAdsPerHour();
  if (ads_per_hour == 0) {
    return;
  }

  const int64_t seconds = base::Time::kSecondsPerHour / ads_per_hour;
  const base::TimeDelta delay = base::TimeDelta::FromSeconds(seconds);
  const base::Time next_interval = MaybeServeAfter(delay);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(next_interval));
}

bool AdServing::NextIntervalHasElapsed() {
  const base::Time now = base::Time::Now();

  const base::Time next_interval = Client::Get()->GetNextAdServingInterval();

  if (now < next_interval) {
    return false;
  }

  return true;
}

base::Time AdServing::MaybeServeAfter(const base::TimeDelta delay) {
  StopServing();

  const base::Time next_interval = base::Time::Now() + delay;
  Client::Get()->SetNextAdServingInterval(next_interval);

  return timer_.Start(
      delay, base::BindOnce(&AdServing::MaybeServe, base::Unretained(this)));
}

void AdServing::MaybeServeAdForSegments(
    const SegmentList& segments,
    MaybeServeAdForSegmentsCallback callback) {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const Result result, const AdEventList& ad_events) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Ad notification not served: Failed to get ad events");
      callback(Result::FAILED, AdNotificationInfo());
      return;
    }

    const int max_count = features::GetBrowsingHistoryMaxCount();
    const int days_ago = features::GetBrowsingHistoryDaysAgo();
    AdsClientHelper::Get()->GetBrowsingHistory(
        max_count, days_ago, [=](const BrowsingHistoryList history) {
          FrequencyCapping frequency_capping(subdivision_targeting_,
                                             anti_targeting_resource_,
                                             ad_events, history);

          if (!frequency_capping.IsAdAllowed()) {
            BLOG(1, "Ad notification not served: Not allowed");
            callback(Result::FAILED, AdNotificationInfo());
            return;
          }

          RecordAdOpportunityForSegments(segments);

          MaybeServeAdForParentChildSegments(segments, ad_events, history,
                                             callback);
        });
  });
}

void AdServing::MaybeServeAdForParentChildSegments(
    const SegmentList& segments,
    const AdEventList& ad_events,
    const BrowsingHistoryList& history,
    MaybeServeAdForSegmentsCallback callback) {
  if (segments.empty()) {
    BLOG(1, "No segments to serve targeted ads");
    MaybeServeAdForUntargeted(ad_events, history, callback);
    return;
  }

  BLOG(1, "Serve ad for segments:");
  for (const auto& segment : segments) {
    BLOG(1, "  " << segment);
  }

  database::table::CreativeAdNotifications database_table;
  database_table.GetForSegments(
      segments, [=](const Result result, const SegmentList& segments,
                    const CreativeAdNotificationList& ads) {
        EligibleAds eligible_ad_notifications(subdivision_targeting_,
                                              anti_targeting_resource_);

        const CreativeAdNotificationList eligible_ads =
            eligible_ad_notifications.Get(ads, last_delivered_creative_ad_,
                                          ad_events, history);
        if (eligible_ads.empty()) {
          BLOG(1, "No eligible ads found for segments");
          MaybeServeAdForParentSegments(segments, ad_events, history, callback);
          return;
        }

        MaybeServeAd(eligible_ads, callback);
      });
}

void AdServing::MaybeServeAdForParentSegments(
    const SegmentList& segments,
    const AdEventList& ad_events,
    const BrowsingHistoryList& history,
    MaybeServeAdForSegmentsCallback callback) {
  const SegmentList parent_segments = GetParentSegments(segments);

  BLOG(1, "Serve ad for parent segments:");
  for (const auto& parent_segment : parent_segments) {
    BLOG(1, "  " << parent_segment);
  }

  database::table::CreativeAdNotifications database_table;
  database_table.GetForSegments(
      parent_segments, [=](const Result result, const SegmentList& segments,
                           const CreativeAdNotificationList& ads) {
        EligibleAds eligible_ad_notifications(subdivision_targeting_,
                                              anti_targeting_resource_);

        const CreativeAdNotificationList eligible_ads =
            eligible_ad_notifications.Get(ads, last_delivered_creative_ad_,
                                          ad_events, history);
        if (eligible_ads.empty()) {
          BLOG(1, "No eligible ads found for parent segments");
          MaybeServeAdForUntargeted(ad_events, history, callback);
          return;
        }

        MaybeServeAd(eligible_ads, callback);
      });
}

void AdServing::MaybeServeAdForUntargeted(
    const AdEventList& ad_events,
    const BrowsingHistoryList& history,
    MaybeServeAdForSegmentsCallback callback) {
  BLOG(1, "Serve untargeted ad");

  const std::vector<std::string> segments = {ad_targeting::kUntargeted};

  database::table::CreativeAdNotifications database_table;
  database_table.GetForSegments(
      segments, [=](const Result result, const SegmentList& segments,
                    const CreativeAdNotificationList& ads) {
        EligibleAds eligible_ad_notifications(subdivision_targeting_,
                                              anti_targeting_resource_);

        const CreativeAdNotificationList eligible_ads =
            eligible_ad_notifications.Get(ads, last_delivered_creative_ad_,
                                          ad_events, history);

        if (eligible_ads.empty()) {
          BLOG(1, "No eligible ads found for untargeted segment");
          BLOG(1, "Ad notification not served: No eligible ads found");
          callback(Result::FAILED, AdNotificationInfo());
          return;
        }

        MaybeServeAd(eligible_ads, callback);
      });
}

void AdServing::MaybeServeAd(const CreativeAdNotificationList& ads,
                             MaybeServeAdForSegmentsCallback callback) {
  CreativeAdNotificationList eligible_ads = PaceAds(ads);
  if (eligible_ads.empty()) {
    BLOG(1, "Ad notification not served: No eligible ads found");
    callback(Result::FAILED, AdNotificationInfo());
    return;
  }

  BLOG(1, "Found " << eligible_ads.size() << " eligible ads");

  const int rand = base::RandInt(0, eligible_ads.size() - 1);
  const CreativeAdNotificationInfo ad = eligible_ads.at(rand);
  MaybeDeliverAd(ad, callback);
}

CreativeAdNotificationList AdServing::PaceAds(
    const CreativeAdNotificationList& ads) {
  BLOG(2, ads.size() << " eligible ads before pacing");

  AdPacing ad_pacing;
  CreativeAdNotificationList paced_ads = ad_pacing.PaceAds(ads);

  BLOG(2, paced_ads.size() << " eligible ads after pacing");

  return paced_ads;
}

void AdServing::MaybeDeliverAd(const CreativeAdNotificationInfo& ad,
                               MaybeServeAdForSegmentsCallback callback) {
  AdNotificationInfo ad_notification;
  ad_notification.type = AdType::kAdNotification;
  ad_notification.uuid = base::GenerateGUID();
  ad_notification.creative_instance_id = ad.creative_instance_id;
  ad_notification.creative_set_id = ad.creative_set_id;
  ad_notification.campaign_id = ad.campaign_id;
  ad_notification.advertiser_id = ad.advertiser_id;
  ad_notification.segment = ad.segment;
  ad_notification.title = ad.title;
  ad_notification.body = ad.body;
  ad_notification.target_url = ad.target_url;

  AdDelivery ad_delivery;
  if (!ad_delivery.MaybeDeliverAd(ad_notification)) {
    BLOG(1, "Ad notification not delivered");
    callback(Result::FAILED, ad_notification);
    return;
  }

  last_delivered_creative_ad_ = ad;

  Client::Get()->UpdateSeenAdvertiser(ad.advertiser_id);

  callback(Result::SUCCESS, ad_notification);
}

void AdServing::FailedToDeliverAd() {
  if (!PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  const base::TimeDelta delay = base::TimeDelta::FromMinutes(2);
  MaybeServeAfter(delay);
}

void AdServing::DeliveredAd() {
  if (!PlatformHelper::GetInstance()->IsMobile()) {
    return;
  }

  MaybeServeNextAd();
}

void AdServing::RecordAdOpportunityForSegments(const SegmentList& segments) {
  const std::vector<std::string> question_list =
      p2a::CreateAdOpportunityQuestionList(segments);

  p2a::RecordEvent("ad_opportunity", question_list);
}

}  // namespace ad_notifications
}  // namespace ads
