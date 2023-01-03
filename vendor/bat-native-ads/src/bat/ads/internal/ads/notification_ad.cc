/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/notification_ad.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/account/account.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_handler.h"
#include "bat/ads/internal/ads/notification_ad_util.h"
#include "bat/ads/internal/ads/serving/notification_ad_serving.h"
#include "bat/ads/internal/browser/browser_manager.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/covariates/covariate_manager.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/history/history_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/internal/privacy/p2a/impressions/p2a_impression.h"
#include "bat/ads/internal/privacy/p2a/opportunities/p2a_opportunity.h"
#include "bat/ads/internal/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_processor.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "bat/ads/internal/transfer/transfer.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_util.h"
#include "bat/ads/notification_ad_info.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads {

NotificationAd::NotificationAd(
    Account* account,
    Transfer* transfer,
    processor::EpsilonGreedyBandit* epsilon_greedy_bandit_processor,
    geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : account_(account),
      transfer_(transfer),
      epsilon_greedy_bandit_processor_(epsilon_greedy_bandit_processor) {
  DCHECK(account_);
  DCHECK(transfer_);
  DCHECK(epsilon_greedy_bandit_processor_);

  account_->AddObserver(this);

  event_handler_ = std::make_unique<notification_ads::EventHandler>();
  event_handler_->AddObserver(this);

  serving_ = std::make_unique<notification_ads::Serving>(
      subdivision_targeting, anti_targeting_resource);
  serving_->AddObserver(this);

  BrowserManager::GetInstance()->AddObserver(this);
  PrefManager::GetInstance()->AddObserver(this);
  IdleDetectionManager::GetInstance()->AddObserver(this);
}

NotificationAd::~NotificationAd() {
  account_->RemoveObserver(this);
  event_handler_->RemoveObserver(this);
  serving_->RemoveObserver(this);
  BrowserManager::GetInstance()->RemoveObserver(this);
  PrefManager::GetInstance()->RemoveObserver(this);
  IdleDetectionManager::GetInstance()->RemoveObserver(this);
}

void NotificationAd::MaybeServeAtRegularIntervals() {
  if (!CanServeAtRegularIntervals()) {
    return;
  }

  if (ShouldServeAtRegularIntervals()) {
    serving_->StartServingAdsAtRegularIntervals();
  } else {
    serving_->StopServingAdsAtRegularIntervals();
  }
}

void NotificationAd::TriggerEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  event_handler_->FireEvent(placement_id, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAd::OnWalletDidUpdate(const WalletInfo& /*wallet*/) {
  MaybeServeAtRegularIntervals();
}

void NotificationAd::OnBrowserDidEnterForeground() {
  MaybeServeAtRegularIntervals();
}

void NotificationAd::OnBrowserDidEnterBackground() {
  MaybeServeAtRegularIntervals();
}

void NotificationAd::OnPrefDidChange(const std::string& path) {
  if (path == prefs::kEnabled) {
    MaybeServeAtRegularIntervals();
  }
}

void NotificationAd::OnUserDidBecomeActive(const base::TimeDelta idle_time,
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

  serving_->MaybeServeAd();
}

void NotificationAd::OnOpportunityAroseToServeNotificationAd(
    const SegmentList& segments) {
  BLOG(1, "Opportunity arose to serve a notification ad");

  privacy::p2a::RecordAdOpportunityForSegments(AdType::kNotificationAd,
                                               segments);
}

void NotificationAd::OnDidServeNotificationAd(const NotificationAdInfo& ad) {
  ShowNotificationAd(ad);

  TriggerEvent(ad.placement_id, mojom::NotificationAdEventType::kServed);
}

void NotificationAd::OnNotificationAdServed(const NotificationAdInfo& ad) {
  ClientStateManager::GetInstance()->UpdateSeenAd(ad);
}

void NotificationAd::OnNotificationAdViewed(const NotificationAdInfo& ad) {
  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kViewed);

  CovariateManager::GetInstance()->SetNotificationAdServedAt(base::Time::Now());

  privacy::p2a::RecordAdImpression(ad);
}

void NotificationAd::OnNotificationAdClicked(const NotificationAdInfo& ad) {
  CloseNotificationAd(ad.placement_id);

  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kClicked);

  processor::EpsilonGreedyBandit::Process(
      {ad.segment, mojom::NotificationAdEventType::kClicked});

  CovariateManager::GetInstance()->SetNotificationAdEvent(
      mojom::NotificationAdEventType::kClicked);
  CovariateManager::GetInstance()->LogTrainingInstance();
}

void NotificationAd::OnNotificationAdDismissed(const NotificationAdInfo& ad) {
  DismissNotificationAd(ad.placement_id);

  HistoryManager::GetInstance()->Add(ad, ConfirmationType::kDismissed);

  account_->Deposit(ad.creative_instance_id, ad.type,
                    ConfirmationType::kDismissed);

  processor::EpsilonGreedyBandit::Process(
      {ad.segment, mojom::NotificationAdEventType::kDismissed});

  CovariateManager::GetInstance()->SetNotificationAdEvent(
      mojom::NotificationAdEventType::kDismissed);
  CovariateManager::GetInstance()->LogTrainingInstance();
}

void NotificationAd::OnNotificationAdTimedOut(const NotificationAdInfo& ad) {
  NotificationAdTimedOut(ad.placement_id);

  processor::EpsilonGreedyBandit::Process(
      {ad.segment, mojom::NotificationAdEventType::kTimedOut});

  CovariateManager::GetInstance()->SetNotificationAdEvent(
      mojom::NotificationAdEventType::kTimedOut);
  CovariateManager::GetInstance()->LogTrainingInstance();
}

}  // namespace ads
