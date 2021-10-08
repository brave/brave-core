/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving.h"

#include <cstdint>

#include "base/check.h"
#include "base/rand_util.h"
#include "base/time/time.h"
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
#include "bat/ads/internal/bundle/creative_ad_notification_info_aliases.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications_base.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications_factory.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/p2a/p2a_ad_opportunities/p2a_ad_opportunity.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segments_aliases.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/time_formatting_util.h"

namespace ads {
namespace ad_notifications {

namespace {

constexpr base::TimeDelta kServeFirstAdAfterDelay =
    base::TimeDelta::FromMinutes(2);

constexpr base::TimeDelta kMinimumDelayBeforeServingAnAd =
    base::TimeDelta::FromMinutes(1);

constexpr base::TimeDelta kRetryServingAdAfterDelay =
    base::TimeDelta::FromMinutes(2);

}  // namespace

AdServing::AdServing(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  const int version = features::GetAdServingVersion();
  eligible_ads_ = EligibleAdsFactory::Build(version, subdivision_targeting,
                                            anti_targeting_resource);
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

  BLOG(1, "Start serving ad notifications at regular intervals");

  const base::TimeDelta delay = CalculateDelayBeforeServingAnAd();

  if (!HasPreviouslyServedAnAd()) {
    const base::Time serve_ad_at = base::Time::Now() + delay;
    Client::Get()->SetServeAdAt(serve_ad_at);
  }

  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(serve_ad_at));
}

void AdServing::StopServingAdsAtRegularIntervals() {
  if (!timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Stop serving ad notifications at regular intervals");

  timer_.Stop();
}

void AdServing::MaybeServeAd() {
  if (is_serving_) {
    BLOG(1, "Already serving ad");
    return;
  }

  is_serving_ = true;

  if (!IsSupported()) {
    BLOG(1, "Ad notification not served: Unsupported version");
    FailedToServeAd();
    return;
  }

  frequency_capping::PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Ad notification not served: Not allowed due to permission rules");
    FailedToServeAd();
    return;
  }

  const ad_targeting::UserModelInfo user_model = ad_targeting::BuildUserModel();

  DCHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, [=](const bool had_opportunity,
                      const CreativeAdNotificationList& creative_ads) {
        if (had_opportunity) {
          const SegmentList segments =
              ad_targeting::GetTopParentChildSegments(user_model);
          p2a::RecordAdOpportunityForSegments(AdType::kAdNotification,
                                              segments);
        }

        if (creative_ads.empty()) {
          BLOG(1, "Ad notification not served: No eligible ads found");
          FailedToServeAd();
          return;
        }

        BLOG(1, "Found " << creative_ads.size() << " eligible ads");

        const int rand = base::RandInt(0, creative_ads.size() - 1);
        const CreativeAdNotificationInfo creative_ad = creative_ads.at(rand);

        const AdNotificationInfo ad = BuildAdNotification(creative_ad);
        if (!ServeAd(ad)) {
          BLOG(1, "Failed to serve ad notification");
          FailedToServeAd();
          return;
        }

        BLOG(1, "Served ad notification");
        ServedAd(ad);
      });
}

void AdServing::OnPrefChanged() {
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

bool AdServing::IsSupported() const {
  if (!eligible_ads_) {
    return false;
  }

  return true;
}

bool AdServing::ShouldServeAdsAtRegularIntervals() const {
  return PlatformHelper::GetInstance()->IsMobile();
}

bool AdServing::HasPreviouslyServedAnAd() const {
  return !Client::Get()->GetServeAdAt().is_null();
}

bool AdServing::ShouldServeAd() const {
  const base::Time serve_ad_at = Client::Get()->GetServeAdAt();
  if (base::Time::Now() < serve_ad_at) {
    return false;
  }

  return true;
}

base::TimeDelta AdServing::CalculateDelayBeforeServingAnAd() const {
  if (!HasPreviouslyServedAnAd()) {
    return kServeFirstAdAfterDelay;
  }

  if (ShouldServeAd()) {
    return kMinimumDelayBeforeServingAnAd;
  }

  return Client::Get()->GetServeAdAt() - base::Time::Now();
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
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(serve_ad_at));
}

void AdServing::RetryServingAdAtNextInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const base::Time serve_ad_at = MaybeServeAdAfter(kRetryServingAdAfterDelay);
  BLOG(1, "Maybe serve ad notification " << FriendlyDateAndTime(serve_ad_at));
}

base::Time AdServing::MaybeServeAdAfter(const base::TimeDelta delay) {
  const base::Time serve_ad_at = base::Time::Now() + delay;
  Client::Get()->SetServeAdAt(serve_ad_at);

  return timer_.Start(
      delay, base::BindOnce(&AdServing::MaybeServeAd, base::Unretained(this)));
}

bool AdServing::ServeAd(const AdNotificationInfo& ad) const {
  DCHECK(ad.IsValid());

  BLOG(1, "Serving ad notification:\n"
              << "  uuid: " << ad.uuid << "\n"
              << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
              << "  creativeSetId: " << ad.creative_set_id << "\n"
              << "  campaignId: " << ad.campaign_id << "\n"
              << "  advertiserId: " << ad.advertiser_id << "\n"
              << "  segment: " << ad.segment << "\n"
              << "  title: " << ad.title << "\n"
              << "  body: " << ad.body << "\n"
              << "  targetUrl: " << ad.target_url);

  AdDelivery ad_delivery;
  if (!ad_delivery.MaybeDeliverAd(ad)) {
    return false;
  }

  NotifyDidServeAdNotification(ad);

  return true;
}

void AdServing::FailedToServeAd() {
  is_serving_ = false;

  NotifyFailedToServeAdNotification();

  RetryServingAdAtNextInterval();
}

void AdServing::ServedAd(const AdNotificationInfo& ad) {
  DCHECK(eligible_ads_);
  eligible_ads_->set_last_served_ad(ad);

  is_serving_ = false;

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
