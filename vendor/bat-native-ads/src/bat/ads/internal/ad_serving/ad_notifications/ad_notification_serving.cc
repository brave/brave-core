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
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/p2a/p2a_ad_opportunities/p2a_ad_opportunity.h"
#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/segments/segments_aliases.h"
#include "bat/ads/internal/segments/segments_util.h"
#include "bat/ads/internal/settings/settings.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace ad_notifications {

namespace {
constexpr base::TimeDelta kRetryServingAdAtNextInterval =
    base::TimeDelta::FromMinutes(2);
}  // namespace

AdServing::AdServing(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : eligible_ads_(std::make_unique<EligibleAds>(subdivision_targeting,
                                                  anti_targeting_resource)) {}

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

  if (Client::Get()->GetServeNextAdAt().is_null()) {
    delay = base::TimeDelta::FromMinutes(2);
    const base::Time next_interval = base::Time::Now() + delay;
    Client::Get()->SetServeNextAdAt(next_interval);
  } else {
    if (ShouldServeAd()) {
      delay = base::TimeDelta::FromMinutes(1);
    } else {
      const base::Time next_interval = Client::Get()->GetServeNextAdAt();

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

  // TODO(https://github.com/brave/brave-browser/issues/17542): Refactor Brave
  // ads serving
  const int ad_serving_version = features::GetAdServingVersion();
  BLOG(1, "Ad serving version " << ad_serving_version);

  switch (ad_serving_version) {
    case 1: {
      MaybeServeAdV1();
      break;
    }

    case 2: {
      MaybeServeAdV2();
      break;
    }

    default: {
      NOTREACHED() << "Ad serving version is not supported";
      break;
    }
  }
}

void AdServing::MaybeServeAdV1() {
  const ad_targeting::UserModelInfo user_model = ad_targeting::BuildUserModel();

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

void AdServing::MaybeServeAdV2() {
  const ad_targeting::UserModelInfo user_model = ad_targeting::BuildUserModel();

  eligible_ads_->GetV2(
      user_model, [=](const bool was_allowed,
                      const absl::optional<CreativeAdNotificationInfo>& ad) {
        if (was_allowed) {
          p2a::RecordAdOpportunityForSegments(AdType::kAdNotification,
                                              user_model.interest_segments);
        }

        if (!ad) {
          BLOG(1, "Ad notification not served: No eligible ads found");
          FailedToServeAd();
          return;
        }

        if (!ServeAd(ad.value())) {
          BLOG(1, "Failed to serve ad notification");
          FailedToServeAd();
          return;
        }

        BLOG(1, "Served ad notification");
        ServedAd(ad.value());
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
  const base::Time next_interval = Client::Get()->GetServeNextAdAt();
  if (base::Time::Now() < next_interval) {
    return false;
  }

  return true;
}

base::Time AdServing::MaybeServeAdAfter(const base::TimeDelta delay) {
  const base::Time next_interval = base::Time::Now() + delay;
  Client::Get()->SetServeNextAdAt(next_interval);

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
