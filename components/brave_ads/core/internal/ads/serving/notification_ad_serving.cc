/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving.h"

#include "base/functional/bind.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_factory.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/notification_ads/notification_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/top_segments.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kRetryServingAdAfterDelay = base::Minutes(2);
}  // namespace

NotificationAdServing::NotificationAdServing(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource) {
  eligible_ads_ = EligibleNotificationAdsFactory::Build(
      kNotificationAdServingVersion.Get(), subdivision_targeting,
      anti_targeting_resource);

  AdsClientHelper::AddObserver(this);
}

NotificationAdServing::~NotificationAdServing() {
  AdsClientHelper::RemoveObserver(this);

  delegate_ = nullptr;
}

void NotificationAdServing::StartServingAdsAtRegularIntervals() {
  if (timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Start serving notification ads at regular intervals");

  const base::TimeDelta delay = CalculateDelayBeforeServingAnAd();
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad "
              << FriendlyDateAndTime(serve_ad_at, /*use_sentence_style*/ true));
}

void NotificationAdServing::StopServingAdsAtRegularIntervals() {
  if (!timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Stop serving notification ads at regular intervals");

  timer_.Stop();
}

void NotificationAdServing::MaybeServeAd() {
  if (is_serving_) {
    BLOG(1, "Already serving notification ad");
    return;
  }

  is_serving_ = true;

  if (!IsNotificationAdServingFeatureEnabled()) {
    BLOG(1, "Notification ad not served: Feature is disabled");
    FailedToServeAd();
    return;
  }

  if (!IsSupported()) {
    BLOG(1, "Notification ad not served: Unsupported version");
    return FailedToServeAd();
  }

  if (!NotificationAdPermissionRules::HasPermission()) {
    BLOG(1, "Notification ad not served: Not allowed due to permission rules");
    return FailedToServeAd();
  }

  BuildUserModel(base::BindOnce(&NotificationAdServing::BuildUserModelCallback,
                                weak_factory_.GetWeakPtr()));
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdServing::BuildUserModelCallback(
    const UserModelInfo& user_model) {
  CHECK(eligible_ads_);
  eligible_ads_->GetForUserModel(
      user_model,
      base::BindOnce(&NotificationAdServing::GetForUserModelCallback,
                     weak_factory_.GetWeakPtr(), user_model));
}

void NotificationAdServing::GetForUserModelCallback(
    const UserModelInfo& user_model,
    const bool had_opportunity,
    const CreativeNotificationAdList& creative_ads) {
  if (had_opportunity) {
    if (delegate_) {
      delegate_->OnOpportunityAroseToServeNotificationAd(
          GetTopChildSegments(user_model));
    }
  }

  if (creative_ads.empty()) {
    BLOG(1, "Notification ad not served: No eligible ads found");
    return FailedToServeAd();
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const int rand = base::RandInt(0, static_cast<int>(creative_ads.size()) - 1);
  const CreativeNotificationAdInfo& creative_ad = creative_ads.at(rand);

  const NotificationAdInfo ad = BuildNotificationAd(creative_ad);
  ServeAd(ad);
}

void NotificationAdServing::OnAdsPerHourPrefChanged() {
  const int ads_per_hour = GetMaximumNotificationAdsPerHourSetting();
  BLOG(1, "Maximum notification ads per hour changed to " << ads_per_hour);

  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  if (ads_per_hour == 0) {
    return StopServingAdsAtRegularIntervals();
  }

  MaybeServeAdAtNextRegularInterval();
}

void NotificationAdServing::MaybeServeAdAtNextRegularInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const int ads_per_hour = GetMaximumNotificationAdsPerHourSetting();
  if (ads_per_hour == 0) {
    return;
  }

  const base::TimeDelta delay = base::Hours(1) / ads_per_hour;
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad "
              << FriendlyDateAndTime(serve_ad_at, /*use_sentence_style*/ true));
}

void NotificationAdServing::RetryServingAdAtNextInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const base::Time serve_ad_at = MaybeServeAdAfter(kRetryServingAdAfterDelay);
  BLOG(1, "Maybe serve notification ad "
              << FriendlyDateAndTime(serve_ad_at, /*use_sentence_style*/ true));
}

base::Time NotificationAdServing::MaybeServeAdAfter(
    const base::TimeDelta delay) {
  const base::Time serve_ad_at = base::Time::Now() + delay;
  SetServeAdAt(serve_ad_at);

  return timer_.Start(FROM_HERE, delay,
                      base::BindOnce(&NotificationAdServing::MaybeServeAd,
                                     base::Unretained(this)));
}

void NotificationAdServing::ServeAd(const NotificationAdInfo& ad) {
  if (!ad.IsValid()) {
    BLOG(1, "Failed to serve notification ad");
    return FailedToServeAd();
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

  is_serving_ = false;

  CHECK(eligible_ads_);
  eligible_ads_->SetLastServedAd(ad);

  if (delegate_) {
    delegate_->OnDidServeNotificationAd(ad);
  }
}

void NotificationAdServing::FailedToServeAd() {
  is_serving_ = false;

  if (delegate_) {
    delegate_->OnFailedToServeNotificationAd();
  }

  RetryServingAdAtNextInterval();
}

void NotificationAdServing::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kMaximumNotificationAdsPerHour) {
    OnAdsPerHourPrefChanged();
  }
}

}  // namespace brave_ads
