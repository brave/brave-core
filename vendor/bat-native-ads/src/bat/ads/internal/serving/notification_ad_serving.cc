/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/notification_ad_serving.h"

#include <cstdint>

#include "base/check.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/platform/platform_helper.h"
#include "bat/ads/internal/base/time/time_formatting_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info_aliases.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/privacy/p2a/opportunities/p2a_opportunity.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segments_aliases.h"
#include "bat/ads/internal/serving/delivery/notification_ads/notification_ad_delivery.h"
#include "bat/ads/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "bat/ads/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_factory.h"
#include "bat/ads/internal/serving/permission_rules/notification_ads/notification_ad_permission_rules.h"
#include "bat/ads/internal/serving/serving_features.h"
#include "bat/ads/internal/serving/targeting/top_segments.h"
#include "bat/ads/internal/serving/targeting/user_model_builder.h"
#include "bat/ads/internal/serving/targeting/user_model_info.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace notification_ads {

namespace {

constexpr base::TimeDelta kServeFirstAdAfterDelay = base::Minutes(2);

constexpr base::TimeDelta kMinimumDelayBeforeServingAnAd = base::Minutes(1);

constexpr base::TimeDelta kRetryServingAdAfterDelay = base::Minutes(2);

}  // namespace

Serving::Serving(geographic::SubdivisionTargeting* subdivision_targeting,
                 resource::AntiTargeting* anti_targeting_resource) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  const int version = features::GetServingVersion();
  eligible_ads_ = EligibleAdsFactory::Build(version, subdivision_targeting,
                                            anti_targeting_resource);
}

Serving::~Serving() = default;

void Serving::AddObserver(NotificationAdServingObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Serving::RemoveObserver(NotificationAdServingObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Serving::OnPrefChanged(const std::string& path) {
  if (path == prefs::kAdsPerHour) {
    OnAdsPerHourPrefChanged();
  }
}

void Serving::StartServingAdsAtRegularIntervals() {
  if (timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Start serving notification ads at regular intervals");

  const base::TimeDelta delay = CalculateDelayBeforeServingAnAd();

  if (!HasPreviouslyServedAnAd()) {
    const base::Time serve_ad_at = base::Time::Now() + delay;
    ClientStateManager::Get()->SetServeAdAt(serve_ad_at);
  }

  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad " << FriendlyDateAndTime(serve_ad_at));
}

void Serving::StopServingAdsAtRegularIntervals() {
  if (!timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Stop serving notification ads at regular intervals");

  timer_.Stop();
}

void Serving::MaybeServeAd() {
  if (is_serving_) {
    BLOG(1, "Already serving ad");
    return;
  }

  is_serving_ = true;

  if (!IsSupported()) {
    BLOG(1, "Notification ad not served: Unsupported version");
    FailedToServeAd();
    return;
  }

  PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Notification ad not served: Not allowed due to permission rules");
    FailedToServeAd();
    return;
  }

  const targeting::UserModelInfo& user_model = targeting::BuildUserModel();

  DCHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, [=](const bool had_opportunity,
                      const CreativeNotificationAdList& creative_ads) {
        if (had_opportunity) {
          const SegmentList& segments =
              targeting::GetTopChildSegments(user_model);
          privacy::p2a::RecordAdOpportunityForSegments(AdType::kNotificationAd,
                                                       segments);
        }

        if (creative_ads.empty()) {
          BLOG(1, "Notification ad not served: No eligible ads found");
          FailedToServeAd();
          return;
        }

        BLOG(1, "Found " << creative_ads.size() << " eligible ads");

        const int rand = base::RandInt(0, creative_ads.size() - 1);
        const CreativeNotificationAdInfo& creative_ad = creative_ads.at(rand);

        const NotificationAdInfo& ad = BuildNotificationAd(creative_ad);
        if (!ServeAd(ad)) {
          BLOG(1, "Failed to serve notification ad");
          FailedToServeAd();
          return;
        }

        BLOG(1, "Served notification ad");
        ServedAd(ad);
      });
}

///////////////////////////////////////////////////////////////////////////////

void Serving::OnAdsPerHourPrefChanged() {
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

bool Serving::IsSupported() const {
  if (!eligible_ads_) {
    return false;
  }

  return true;
}

bool Serving::ShouldServeAdsAtRegularIntervals() const {
  return PlatformHelper::GetInstance()->IsMobile();
}

bool Serving::HasPreviouslyServedAnAd() const {
  return !ClientStateManager::Get()->GetServeAdAt().is_null();
}

bool Serving::ShouldServeAd() const {
  const base::Time serve_ad_at = ClientStateManager::Get()->GetServeAdAt();
  if (base::Time::Now() < serve_ad_at) {
    return false;
  }

  return true;
}

base::TimeDelta Serving::CalculateDelayBeforeServingAnAd() const {
  if (!HasPreviouslyServedAnAd()) {
    return kServeFirstAdAfterDelay;
  }

  if (ShouldServeAd()) {
    return kMinimumDelayBeforeServingAnAd;
  }

  base::TimeDelta delay =
      ClientStateManager::Get()->GetServeAdAt() - base::Time::Now();
  if (delay.is_negative()) {
    delay = base::TimeDelta();
  }

  return delay;
}

void Serving::MaybeServeAdAtNextRegularInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const int64_t ads_per_hour = settings::GetAdsPerHour();
  if (ads_per_hour == 0) {
    return;
  }

  const int64_t seconds = base::Time::kSecondsPerHour / ads_per_hour;
  const base::TimeDelta delay = base::Seconds(seconds);
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad " << FriendlyDateAndTime(serve_ad_at));
}

void Serving::RetryServingAdAtNextInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const base::Time serve_ad_at = MaybeServeAdAfter(kRetryServingAdAfterDelay);
  BLOG(1, "Maybe serve notification ad " << FriendlyDateAndTime(serve_ad_at));
}

base::Time Serving::MaybeServeAdAfter(const base::TimeDelta delay) {
  const base::Time serve_ad_at = base::Time::Now() + delay;
  ClientStateManager::Get()->SetServeAdAt(serve_ad_at);

  return timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Serving::MaybeServeAd, base::Unretained(this)));
}

bool Serving::ServeAd(const NotificationAdInfo& ad) const {
  DCHECK(ad.IsValid());

  BLOG(1, "Serving notification ad:\n"
              << "  placementId: " << ad.placement_id << "\n"
              << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
              << "  creativeSetId: " << ad.creative_set_id << "\n"
              << "  campaignId: " << ad.campaign_id << "\n"
              << "  advertiserId: " << ad.advertiser_id << "\n"
              << "  segment: " << ad.segment << "\n"
              << "  title: " << ad.title << "\n"
              << "  body: " << ad.body << "\n"
              << "  targetUrl: " << ad.target_url);

  Delivery delivery;
  if (!delivery.MaybeDeliverAd(ad)) {
    return false;
  }

  NotifyDidServeNotificationAd(ad);

  return true;
}

void Serving::FailedToServeAd() {
  is_serving_ = false;

  NotifyFailedToServeNotificationAd();

  RetryServingAdAtNextInterval();
}

void Serving::ServedAd(const NotificationAdInfo& ad) {
  DCHECK(eligible_ads_);
  eligible_ads_->set_last_served_ad(ad);

  is_serving_ = false;

  MaybeServeAdAtNextRegularInterval();
}

void Serving::NotifyDidServeNotificationAd(const NotificationAdInfo& ad) const {
  for (NotificationAdServingObserver& observer : observers_) {
    observer.OnDidServeNotificationAd(ad);
  }
}

void Serving::NotifyFailedToServeNotificationAd() const {
  for (NotificationAdServingObserver& observer : observers_) {
    observer.OnFailedToServeNotificationAd();
  }
}

}  // namespace notification_ads
}  // namespace ads
