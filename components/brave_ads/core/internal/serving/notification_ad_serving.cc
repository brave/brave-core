/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving.h"

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_base.h"
#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_factory.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_feature.h"
#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/notification_ads/notification_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_builder.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

NotificationAdServing::NotificationAdServing(
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource) {
  eligible_ads_ = EligibleNotificationAdsFactory::Build(
      kNotificationAdServingVersion.Get(), subdivision_targeting,
      anti_targeting_resource);

  GetAdsClient()->AddObserver(this);
}

NotificationAdServing::~NotificationAdServing() {
  GetAdsClient()->RemoveObserver(this);
  delegate_ = nullptr;
}

void NotificationAdServing::StartServingAdsAtRegularIntervals() {
  if (timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Start serving notification ads at regular intervals");

  const base::TimeDelta delay = CalculateDelayBeforeServingAnAd();
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad " << FriendlyDateAndTime(serve_ad_at));
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
    return BLOG(1, "Already serving notification ad");
  }

  is_serving_ = true;

  const auto result = CanServeAd();
  if (!result.has_value()) {
    BLOG(1, result.error());
    return FailedToServeAd();
  }

  GetEligibleAds();
}

///////////////////////////////////////////////////////////////////////////////

base::expected<void, std::string> NotificationAdServing::CanServeAd() const {
  if (!base::FeatureList::IsEnabled(kNotificationAdServingFeature)) {
    return base::unexpected("Notification ad not served: Feature is disabled");
  }

  if (!IsSupported()) {
    return base::unexpected("Notification ad not served: Unsupported version");
  }

  if (!NotificationAdPermissionRules::HasPermission()) {
    return base::unexpected(
        "Notification ad not served: Not allowed due to permission rules");
  }

  return base::ok();
}

void NotificationAdServing::GetEligibleAds() {
  const UserModelInfo user_model = BuildUserModel();

  NotifyOpportunityAroseToServeNotificationAd(user_model.interest.segments);

  eligible_ads_->GetForUserModel(
      user_model, base::BindOnce(&NotificationAdServing::GetEligibleAdsCallback,
                                 weak_factory_.GetWeakPtr()));
}

void NotificationAdServing::GetEligibleAdsCallback(
    const CreativeNotificationAdList& creative_ads) {
  if (creative_ads.empty()) {
    BLOG(1, "Notification ad not served: No eligible ads found");
    return FailedToServeAd();
  }

  BLOG(1, "Found " << creative_ads.size() << " eligible ads");

  const CreativeNotificationAdInfo creative_ad = ChooseCreativeAd(creative_ads);
  BLOG(1, "Chosen eligible ad with creative instance id "
              << creative_ad.creative_instance_id << " and a priority of "
              << creative_ad.priority);

  ServeAd(BuildNotificationAd(creative_ad));
}

void NotificationAdServing::UpdateMaximumAdsPerHour() {
  BLOG(1, "Maximum notification ads per hour changed to "
              << GetMaximumNotificationAdsPerHour());

  MaybeServeAdAtNextRegularInterval();
}

void NotificationAdServing::MaybeServeAdAtNextRegularInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const base::TimeDelta delay =
      base::Hours(1) / GetMaximumNotificationAdsPerHour();
  const base::Time serve_ad_at = MaybeServeAdAfter(delay);
  BLOG(1, "Maybe serve notification ad " << FriendlyDateAndTime(serve_ad_at));
}

void NotificationAdServing::RetryServingAdAtNextInterval() {
  if (!ShouldServeAdsAtRegularIntervals()) {
    return;
  }

  const base::Time serve_ad_at =
      MaybeServeAdAfter(kRetryServingNotificationAdAfter.Get());
  BLOG(1, "Maybe serve notification ad " << FriendlyDateAndTime(serve_ad_at));
}

base::Time NotificationAdServing::MaybeServeAdAfter(
    const base::TimeDelta delay) {
  SetServeAdAt(base::Time::Now() + delay);

  return timer_.Start(FROM_HERE, delay,
                      base::BindOnce(&NotificationAdServing::MaybeServeAd,
                                     weak_factory_.GetWeakPtr()));
}

void NotificationAdServing::ServeAd(const NotificationAdInfo& ad) {
  if (!ad.IsValid()) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Detect potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Invalid notification ad");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Failed to serve notification ad due to the ad being invalid");

    return FailedToServeAd();
  }

  eligible_ads_->SetLastServedAd(ad);

  SuccessfullyServedAd(ad);
}

void NotificationAdServing::SuccessfullyServedAd(const NotificationAdInfo& ad) {
  is_serving_ = false;

  NotifyDidServeNotificationAd(ad);
}

void NotificationAdServing::FailedToServeAd() {
  is_serving_ = false;

  NotifyFailedToServeNotificationAd();

  RetryServingAdAtNextInterval();
}

void NotificationAdServing::NotifyOpportunityAroseToServeNotificationAd(
    const SegmentList& segments) const {
  if (delegate_) {
    delegate_->OnOpportunityAroseToServeNotificationAd(segments);
  }
}

void NotificationAdServing::NotifyDidServeNotificationAd(
    const NotificationAdInfo& ad) const {
  if (delegate_) {
    delegate_->OnDidServeNotificationAd(ad);
  }
}

void NotificationAdServing::NotifyFailedToServeNotificationAd() const {
  if (delegate_) {
    delegate_->OnFailedToServeNotificationAd();
  }
}

void NotificationAdServing::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kMaximumNotificationAdsPerHour) {
    UpdateMaximumAdsPerHour();
  }
}

}  // namespace brave_ads
