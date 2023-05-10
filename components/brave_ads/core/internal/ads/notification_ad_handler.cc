/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/notification_ad_handler.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/ads/notification_ad_handler_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_event_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/variables/notification_ad_served_at_predictor_variable_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/impressions/p2a_impression.h"
#include "brave/components/brave_ads/core/internal/privacy/p2a/opportunities/p2a_opportunity.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feedback_info.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_idle_detection/user_idle_detection_util.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"

namespace brave_ads {

NotificationAdHandler::NotificationAdHandler(
    Account& account,
    Transfer& transfer,
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : account_(account),
      transfer_(transfer),
      serving_(subdivision_targeting, anti_targeting_resource) {
  account_->AddObserver(this);

  event_handler_.SetDelegate(this);

  serving_.SetDelegate(this);

  BrowserManager::GetInstance().AddObserver(this);
  AdsClientHelper::AddObserver(this);
}

NotificationAdHandler::~NotificationAdHandler() {
  account_->RemoveObserver(this);
  BrowserManager::GetInstance().RemoveObserver(this);
  AdsClientHelper::RemoveObserver(this);
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
    const mojom::NotificationAdEventType event_type) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_.FireEvent(placement_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdHandler::OnWalletDidUpdate(const WalletInfo& /*wallet*/) {
  MaybeServeAtRegularIntervals();
}

void NotificationAdHandler::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kEnabled) {
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
    BLOG(1, "Notification ad not served: Screen was locked");
    return;
  }

  if (HasExceededMaximumIdleTime(idle_time)) {
    BLOG(1, "Notification ad not served: Exceeded maximum idle time");
    return;
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

  privacy::p2a::RecordAdOpportunityForSegments(AdType::kNotificationAd,
                                               segments);
}

void NotificationAdHandler::OnDidServeNotificationAd(
    const NotificationAdInfo& ad) {
  ShowNotificationAd(ad);

  TriggerEvent(ad.placement_id, mojom::NotificationAdEventType::kServed);

  serving_.MaybeServeAdAtNextRegularInterval();
}

void NotificationAdHandler::OnDidFireNotificationAdServedEvent(
    const NotificationAdInfo& ad) {
  ClientStateManager::GetInstance().UpdateSeenAd(ad);
}

void NotificationAdHandler::OnDidFireNotificationAdViewedEvent(
    const NotificationAdInfo& ad) {
  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kViewed);

  SetNotificationAdServedAtPredictorVariable(base::Time::Now());

  privacy::p2a::RecordAdImpression(ad);
}

void NotificationAdHandler::OnDidFireNotificationAdClickedEvent(
    const NotificationAdInfo& ad) {
  CloseNotificationAd(ad.placement_id);

  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kClicked);

  EpsilonGreedyBanditProcessor::Process(
      {ad.segment, mojom::NotificationAdEventType::kClicked});

  SetNotificationAdEventPredictorVariable(
      mojom::NotificationAdEventType::kClicked);
  PredictorsManager::GetInstance().AddTrainingSample();
}

void NotificationAdHandler::OnDidFireNotificationAdDismissedEvent(
    const NotificationAdInfo& ad) {
  DismissNotificationAd(ad.placement_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kDismissed);

  account_->Deposit(ad.creative_instance_id, ad.type, ad.segment,
                    ConfirmationType::kDismissed);

  EpsilonGreedyBanditProcessor::Process(
      {ad.segment, mojom::NotificationAdEventType::kDismissed});

  SetNotificationAdEventPredictorVariable(
      mojom::NotificationAdEventType::kDismissed);
  PredictorsManager::GetInstance().AddTrainingSample();
}

void NotificationAdHandler::OnDidFireNotificationAdTimedOutEvent(
    const NotificationAdInfo& ad) {
  NotificationAdTimedOut(ad.placement_id);
  EpsilonGreedyBanditProcessor::Process(
      {ad.segment, mojom::NotificationAdEventType::kTimedOut});

  SetNotificationAdEventPredictorVariable(
      mojom::NotificationAdEventType::kTimedOut);
  PredictorsManager::GetInstance().AddTrainingSample();
}

}  // namespace brave_ads
