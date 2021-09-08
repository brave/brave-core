/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"

#include <cstdint>

#include "base/check.h"
#include "base/rand_util.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/internal/ad_delivery/ad_notifications/ad_notification_delivery.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_builder.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_user_model_info.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification_builder.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notification_permission_rules.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/p2a/p2a_ad_opportunities/p2a_ad_opportunity.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segments_alias.h"
#include "bat/ads/internal/segments/segments_util.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/time_formatting_util.h"

namespace ads {
namespace ad_notifications {

namespace {
constexpr base::TimeDelta kRetryServingAdAtNextInterval =
    base::TimeDelta::FromMinutes(2);
}  // namespace

AdServing::AdServing(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : subdivision_targeting_(subdivision_targeting),
      anti_targeting_resource_(anti_targeting_resource),
      eligible_ads_(std::make_unique<EligibleAds>(subdivision_targeting,
                                                  anti_targeting_resource)) {
  DCHECK(subdivision_targeting_);
  DCHECK(anti_targeting_resource_);
}

AdServing::~AdServing() = default;

void AdServing::AddObserver(AdNotificationServingObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void AdServing::RemoveObserver(AdNotificationServingObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdServing::StartServingAdsAtRegularIntervals() {
  if (timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Start serving ads at regular intervals");

  base::TimeDelta delay;

  if (Client::Get()->GetNextAdServingInterval().is_null()) {
    delay = base::TimeDelta::FromMinutes(2);
    const base::Time next_interval = base::Time::Now() + delay;
    Client::Get()->SetNextAdServingInterval(next_interval);
  } else {
    if (ShouldServeAd()) {
      delay = base::TimeDelta::FromMinutes(1);
    } else {
      const base::Time next_interval =
          Client::Get()->GetNextAdServingInterval();

      delay = next_interval - base::Time::Now();
    }
  }

  const base::Time next_interval = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(next_interval));
}

void AdServing::StopServingAdsAtRegularIntervals() {
  if (!timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Stop serving ads at regular intervals");

  timer_.Stop();
}

void AdServing::MaybeServeAd() {
  frequency_capping::PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Ad notification not served: Not allowed due to permission rules");
    FailedToServeAd();
    return;
  }

  const ad_targeting::UserModelInfo user_model = ad_targeting::BuildUserModel();

  DCHECK(eligible_ads_);
  eligible_ads_->Get(user_model, [=](const bool was_allowed,
                                     const CreativeAdNotificationList& ads) {
    if (was_allowed) {
      const SegmentList segments =
          ad_targeting::GetTopParentChildSegments(user_model);
      p2a::RecordAdOpportunityForSegments(AdType::kAdNotification, segments);
    }

    if (ads.empty()) {
      BLOG(1, "Ad notification not served: No eligible ads found");
      FailedToServeAd();
      return;
    }

    BLOG(1, "Found " << ads.size() << " eligible ads");

    const int rand = base::RandInt(0, ads.size() - 1);
    const CreativeAdNotificationInfo ad = ads.at(rand);

    if (!ServeAd(ad)) {
      BLOG(1, "Failed to serve ad notification");
      FailedToServeAd();
      return;
    }

    BLOG(1, "Served ad notification");
    ServedAd(ad);
  });
}

void AdServing::OnAdsPerHourChanged() {
  const int64_t ads_per_hour = settings::GetAdsPerHour();
  BLOG(1, "Maximum ads per hour changed to " << ads_per_hour);

  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  if (ads_per_hour == 0) {
    StopServingAdsAtRegularIntervals();
    return;
  }

  MaybeServeAdAtNextRegularInterval();
}

///////////////////////////////////////////////////////////////////////////////

bool AdServing::ShouldServeAdsAtRegularIntervals() const {
  return PlatformHelper::GetInstance()->IsMobile();
}

void AdServing::MaybeServeAdAtNextRegularInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const int64_t ads_per_hour = settings::GetAdsPerHour();
  if (ads_per_hour == 0) {
    return;
  }

  const int64_t seconds = base::Time::kSecondsPerHour / ads_per_hour;
  const base::TimeDelta delay = base::TimeDelta::FromSeconds(seconds);
  const base::Time next_interval = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(next_interval));
}

void AdServing::RetryServingAdAtNextInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const base::Time next_interval =
      MaybeServeAdAfter(kRetryServingAdAtNextInterval);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(next_interval));
}

bool AdServing::ShouldServeAd() const {
  const base::Time next_interval = Client::Get()->GetNextAdServingInterval();
  if (base::Time::Now() < next_interval) {
    return false;
  }

  return true;
}

base::Time AdServing::MaybeServeAdAfter(const base::TimeDelta delay) {
  const base::Time next_interval = base::Time::Now() + delay;
  Client::Get()->SetNextAdServingInterval(next_interval);

  return timer_.Start(
      delay, base::BindOnce(&AdServing::MaybeServeAd, base::Unretained(this)));
}

bool AdServing::ServeAd(
    const CreativeAdNotificationInfo& creative_ad_notification) const {
  const AdNotificationInfo ad_notification =
      BuildAdNotification(creative_ad_notification);

  BLOG(1, "Serving ad notification:\n"
              << "  uuid: " << ad_notification.uuid << "\n"
              << "  creativeInstanceId: "
              << ad_notification.creative_instance_id << "\n"
              << "  creativeSetId: " << ad_notification.creative_set_id << "\n"
              << "  campaignId: " << ad_notification.campaign_id << "\n"
              << "  advertiserId: " << ad_notification.advertiser_id << "\n"
              << "  segment: " << ad_notification.segment << "\n"
              << "  title: " << ad_notification.title << "\n"
              << "  body: " << ad_notification.body << "\n"
              << "  targetUrl: " << ad_notification.target_url);

  AdDelivery ad_delivery;
  if (!ad_delivery.MaybeDeliverAd(ad_notification)) {
    return false;
  }

  NotifyDidServeAdNotification(ad_notification);

  return true;
}

void AdServing::FailedToServeAd() {
  NotifyFailedToServeAdNotification();

  RetryServingAdAtNextInterval();
}

void AdServing::ServedAd(
    const CreativeAdNotificationInfo& creative_ad_notification) {
  eligible_ads_->SetLastServedAd(creative_ad_notification);

  MaybeServeAdAtNextRegularInterval();
}

void AdServing::NotifyDidServeAdNotification(
    const AdNotificationInfo& ad) const {
  for (AdNotificationServingObserver& observer : observers_) {
    observer.OnDidServeAdNotification(ad);
  }
}

void AdServing::NotifyFailedToServeAdNotification() const {
  for (AdNotificationServingObserver& observer : observers_) {
    observer.OnFailedToServeAdNotification();
  }
}

}  // namespace ad_notifications
}  // namespace ads
