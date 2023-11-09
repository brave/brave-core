/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/notification_ad/notification_ad_handler.h"

#include <utility>

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_event_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_served_at_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/internal/units/notification_ad/notification_ad_handler_util.h"
#include "brave/components/brave_ads/core/internal/user/user_attention/user_idle_detection/user_idle_detection_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_info.h"

namespace brave_ads {

namespace {

void FireEventCallback(TriggerAdEventCallback callback,
                       const bool success,
                       const std::string& /*placement_id=*/,
                       const mojom::NotificationAdEventType /*event_type=*/) {
  std::move(callback).Run(success);
}

void MaybeCloseAllNotifications() {
  if (!UserHasOptedInToNotificationAds()) {
    NotificationAdManager::GetInstance().RemoveAll(/*should_close=*/true);
  }
}

}  // namespace

NotificationAdHandler::NotificationAdHandler(
    Account& account,
    Transfer& transfer,
    EpsilonGreedyBanditProcessor& epsilon_greedy_bandit_processor,
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : account_(account),
      transfer_(transfer),
      epsilon_greedy_bandit_processor_(epsilon_greedy_bandit_processor),
      serving_(subdivision_targeting, anti_targeting_resource) {
  AddAdsClientNotifierObserver(this);
  BrowserManager::GetInstance().AddObserver(this);
  event_handler_.SetDelegate(this);
  serving_.SetDelegate(this);
}

NotificationAdHandler::~NotificationAdHandler() {
  RemoveAdsClientNotifierObserver(this);
  BrowserManager::GetInstance().RemoveObserver(this);
}

void NotificationAdHandler::MaybeServeAtRegularIntervals() {
  if (!CanServeAtRegularIntervals()) {
    return;
  }

  if (ShouldServeAtRegularIntervals()) {
    serving_.StartServingAdsAtRegularIntervals();
  } else {
    serving_.StopServingAdsAtRegularIntervals();
  }
}

void NotificationAdHandler::TriggerEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));
  CHECK_NE(mojom::NotificationAdEventType::kServed, event_type)
      << "Should not be called with kServed as this event is handled when "
         "calling TriggerEvent with kViewed";

  if (!UserHasOptedInToNotificationAds()) {
    return std::move(callback).Run(/*success=*/false);
  }

  if (event_type == mojom::NotificationAdEventType::kViewed) {
    return event_handler_.FireEvent(
        placement_id, mojom::NotificationAdEventType::kServed,
        base::BindOnce(&NotificationAdHandler::FireServedEventCallback,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
  }

  event_handler_.FireEvent(
      placement_id, event_type,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdHandler::FireServedEventCallback(
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& placement_id,
    const mojom::NotificationAdEventType /*event_type=*/) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  event_handler_.FireEvent(
      placement_id, mojom::NotificationAdEventType::kViewed,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

void NotificationAdHandler::OnNotifyDidInitializeAds() {
  MaybeServeAtRegularIntervals();
}

void NotificationAdHandler::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kOptedInToNotificationAds) {
    MaybeCloseAllNotifications();

    MaybeServeAtRegularIntervals();
  }
}

void NotificationAdHandler::OnNotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) {
  if (!CanServeIfUserIsActive() || !ShouldServe()) {
    return;
  }

  if (MaybeScreenWasLocked(screen_was_locked)) {
    return BLOG(1, "Notification ad not served: Screen was locked");
  }

  if (HasExceededMaximumIdleTime(idle_time)) {
    return BLOG(1, "Notification ad not served: Exceeded maximum idle time");
  }

  serving_.MaybeServeAd();
}

void NotificationAdHandler::OnBrowserDidEnterForeground() {
  MaybeServeAtRegularIntervals();
}

void NotificationAdHandler::OnBrowserDidEnterBackground() {
  MaybeServeAtRegularIntervals();
}

void NotificationAdHandler::OnOpportunityAroseToServeNotificationAd(
    const SegmentList& segments) {
  BLOG(1, "Opportunity arose to serve a notification ad");

  RecordP2AAdOpportunity(AdType::kNotificationAd, segments);
}

void NotificationAdHandler::OnDidServeNotificationAd(
    const NotificationAdInfo& ad) {
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

  NotificationAdManager::GetInstance().Add(ad);

  serving_.MaybeServeAdAtNextRegularInterval();
}

void NotificationAdHandler::OnDidFireNotificationAdServedEvent(
    const NotificationAdInfo& ad) {
  BLOG(3, "Served notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  ClientStateManager::GetInstance().UpdateSeenAd(ad);
}

void NotificationAdHandler::OnDidFireNotificationAdViewedEvent(
    const NotificationAdInfo& ad) {
  BLOG(3, "Viewed notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kViewed);

  SetNotificationAdServedAtPredictorVariable(base::Time::Now());
}

void NotificationAdHandler::OnDidFireNotificationAdClickedEvent(
    const NotificationAdInfo& ad) {
  BLOG(3, "Clicked notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  NotificationAdManager::GetInstance().Remove(ad.placement_id,
                                              /*should_close=*/true);

  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kClicked);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::NotificationAdEventType::kClicked});

  SetNotificationAdEventPredictorVariable(
      mojom::NotificationAdEventType::kClicked);
  PredictorsManager::GetInstance().AddTrainingSample();
}

void NotificationAdHandler::OnDidFireNotificationAdDismissedEvent(
    const NotificationAdInfo& ad) {
  BLOG(3, "Dismissed notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  NotificationAdManager::GetInstance().Remove(ad.placement_id,
                                              /*should_close=*/false);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kDismissed);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kDismissed);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::NotificationAdEventType::kDismissed});

  SetNotificationAdEventPredictorVariable(
      mojom::NotificationAdEventType::kDismissed);
  PredictorsManager::GetInstance().AddTrainingSample();
}

void NotificationAdHandler::OnDidFireNotificationAdTimedOutEvent(
    const NotificationAdInfo& ad) {
  BLOG(3, "Timed out notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  NotificationAdManager::GetInstance().Remove(ad.placement_id,
                                              /*should_close=*/false);

  epsilon_greedy_bandit_processor_->Process(
      {ad.segment, mojom::NotificationAdEventType::kTimedOut});

  SetNotificationAdEventPredictorVariable(
      mojom::NotificationAdEventType::kTimedOut);
  PredictorsManager::GetInstance().AddTrainingSample();
}

}  // namespace brave_ads
