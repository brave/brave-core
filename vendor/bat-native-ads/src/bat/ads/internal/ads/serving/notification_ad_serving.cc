/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/notification_ad_serving.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "bat/ads/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_factory.h"
#include "bat/ads/internal/ads/serving/notification_ad_serving_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/notification_ads/notification_ad_permission_rules.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/ads/serving/targeting/top_segments.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_builder.h"
#include "bat/ads/internal/ads/serving/targeting/user_model_info.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/notification_ad_info.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::notification_ads {

namespace {
constexpr base::TimeDelta kRetryServingAdAfterDelay = base::Minutes(2);
}  // namespace

Serving::Serving(geographic::SubdivisionTargeting* subdivision_targeting,
                 resource::AntiTargeting* anti_targeting_resource) {
  DCHECK(subdivision_targeting);
  DCHECK(anti_targeting_resource);

  const int version = features::GetServingVersion();
  eligible_ads_ = EligibleAdsFactory::Build(version, subdivision_targeting,
                                            anti_targeting_resource);

  PrefManager::GetInstance()->AddObserver(this);
}

Serving::~Serving() {
  PrefManager::GetInstance()->RemoveObserver(this);
}

void Serving::AddObserver(ServingObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Serving::RemoveObserver(ServingObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Serving::StartServingAdsAtRegularIntervals() {
  if (timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Start serving notification ads at regular intervals");

  const base::TimeDelta delay = CalculateDelayBeforeServingAnAd();
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad "
              << FriendlyDateAndTime(serve_ad_at, /*use_sentence_style*/ true));
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
    BLOG(1, "Already serving notification ad");
    return;
  }

  is_serving_ = true;

  if (!IsSupported()) {
    BLOG(1, "Notification ad not served: Unsupported version");
    FailedToServeAd();
    return;
  }

  if (!PermissionRules::HasPermission()) {
    BLOG(1, "Notification ad not served: Not allowed due to permission rules");
    FailedToServeAd();
    return;
  }

  const targeting::UserModelInfo user_model = targeting::BuildUserModel();

  DCHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model, base::BindOnce(&Serving::OnGetForUserModel,
                                 base::Unretained(this), user_model));
}

///////////////////////////////////////////////////////////////////////////////

void Serving::OnGetForUserModel(
    const targeting::UserModelInfo& user_model,
    const bool had_opportunity,
    const CreativeNotificationAdList& creative_ads) {
  if (had_opportunity) {
    const SegmentList segments = targeting::GetTopChildSegments(user_model);
    NotifyOpportunityAroseToServeNotificationAd(segments);
  }

  if (creative_ads.empty()) {
    BLOG(1, "Notification ad not served: No eligible ads found");
    FailedToServeAd();
    return;
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const int rand = base::RandInt(0, static_cast<int>(creative_ads.size()) - 1);
  const CreativeNotificationAdInfo& creative_ad = creative_ads.at(rand);

  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  ServeAd(ad);
}

bool Serving::IsSupported() const {
  return static_cast<bool>(eligible_ads_);
}

void Serving::MaybeServeAdAtNextRegularInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();
  if (ads_per_hour == 0) {
    return;
  }

  const base::TimeDelta delay =
      base::Seconds(base::Time::kSecondsPerHour / ads_per_hour);
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad "
              << FriendlyDateAndTime(serve_ad_at, /*use_sentence_style*/ true));
}

void Serving::RetryServingAdAtNextInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const base::Time serve_ad_at = MaybeServeAdAfter(kRetryServingAdAfterDelay);
  BLOG(1, "Maybe serve notification ad "
              << FriendlyDateAndTime(serve_ad_at, /*use_sentence_style*/ true));
}

base::Time Serving::MaybeServeAdAfter(const base::TimeDelta delay) {
  const base::Time serve_ad_at = base::Time::Now() + delay;
  SetServeAdAt(serve_ad_at);

  return timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&Serving::MaybeServeAd, base::Unretained(this)));
}

void Serving::ServeAd(const NotificationAdInfo& ad) {
  if (!ad.IsValid()) {
    BLOG(1, "Failed to serve notification ad");
    FailedToServeAd();
    return;
  }

  BLOG(1, "Served notification ad:\n"
              << "  placementId: " << ad.placement_id << "\n"
              << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
              << "  creativeSetId: " << ad.creative_set_id << "\n"
              << "  campaignId: " << ad.campaign_id << "\n"
              << "  advertiserId: " << ad.advertiser_id << "\n"
              << "  segment: " << ad.segment << "\n"
              << "  title: " << ad.title << "\n"
              << "  body: " << ad.body << "\n"
              << "  targetUrl: " << ad.target_url);

  DCHECK(eligible_ads_);
  eligible_ads_->SetLastServedAd(ad);

  is_serving_ = false;

  NotifyDidServeNotificationAd(ad);

  MaybeServeAdAtNextRegularInterval();
}

void Serving::FailedToServeAd() {
  is_serving_ = false;

  NotifyFailedToServeNotificationAd();

  RetryServingAdAtNextInterval();
}

void Serving::NotifyOpportunityAroseToServeNotificationAd(
    const SegmentList& segments) const {
  for (ServingObserver& observer : observers_) {
    observer.OnOpportunityAroseToServeNotificationAd(segments);
  }
}

void Serving::NotifyDidServeNotificationAd(const NotificationAdInfo& ad) const {
  for (ServingObserver& observer : observers_) {
    observer.OnDidServeNotificationAd(ad);
  }
}

void Serving::NotifyFailedToServeNotificationAd() const {
  for (ServingObserver& observer : observers_) {
    observer.OnFailedToServeNotificationAd();
  }
}

void Serving::OnPrefDidChange(const std::string& path) {
  if (path == prefs::kMaximumNotificationAdsPerHour) {
    OnAdsPerHourPrefChanged();
  }
}

void Serving::OnAdsPerHourPrefChanged() {
  const int ads_per_hour = settings::GetMaximumNotificationAdsPerHour();
  BLOG(1, "Maximum notification ads per hour changed to " << ads_per_hour);

  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  if (ads_per_hour == 0) {
    StopServingAdsAtRegularIntervals();
    return;
  }

  MaybeServeAdAtNextRegularInterval();
}

}  // namespace ads::notification_ads
