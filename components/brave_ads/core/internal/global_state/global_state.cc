/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/ads_client.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"
#include "brave/components/brave_ads/core/internal/flags/flag_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state_holder.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"

namespace brave_ads {

GlobalState::GlobalState(AdsClient* ads_client)
    : ads_client_(ads_client),
      global_state_holder_(std::make_unique<GlobalStateHolder>(this)) {
  DCHECK(ads_client_);

  browser_manager_ = std::make_unique<BrowserManager>();
  client_state_manager_ = std::make_unique<ClientStateManager>();
  confirmation_state_manager_ = std::make_unique<ConfirmationStateManager>();
  predictors_manager_ = std::make_unique<PredictorsManager>();
  database_manager_ = std::make_unique<DatabaseManager>();
  diagnostic_manager_ = std::make_unique<DiagnosticManager>();
  flag_manager_ = std::make_unique<FlagManager>();
  history_manager_ = std::make_unique<HistoryManager>();
  idle_detection_ = std::make_unique<IdleDetection>();
  notification_ad_manager_ = std::make_unique<NotificationAdManager>();
  tab_manager_ = std::make_unique<TabManager>();
  user_activity_manager_ = std::make_unique<UserActivityManager>();
}

GlobalState::~GlobalState() = default;

// static
GlobalState* GlobalState::GetInstance() {
  DCHECK(GlobalStateHolder::GetGlobalState());
  return GlobalStateHolder::GetGlobalState();
}

// static
bool GlobalState::HasInstance() {
  return !!GlobalStateHolder::GetGlobalState();
}

AdsClient* GlobalState::GetAdsClient() {
  DCHECK(ads_client_);
  return ads_client_;
}

BrowserManager* GlobalState::GetBrowserManager() {
  return browser_manager_.get();
}

ClientStateManager* GlobalState::GetClientStateManager() {
  return client_state_manager_.get();
}

ConfirmationStateManager* GlobalState::GetConfirmationStateManager() {
  return confirmation_state_manager_.get();
}

DatabaseManager* GlobalState::GetDatabaseManager() {
  return database_manager_.get();
}

DiagnosticManager* GlobalState::GetDiagnosticManager() {
  return diagnostic_manager_.get();
}

FlagManager* GlobalState::GetFlagManager() {
  return flag_manager_.get();
}

HistoryManager* GlobalState::GetHistoryManager() {
  return history_manager_.get();
}

NotificationAdManager* GlobalState::GetNotificationAdManager() {
  return notification_ad_manager_.get();
}

PredictorsManager* GlobalState::GetPredictorsManager() {
  return predictors_manager_.get();
}

TabManager* GlobalState::GetTabManager() {
  return tab_manager_.get();
}

UserActivityManager* GlobalState::GetUserActivityManager() {
  return user_activity_manager_.get();
}

mojom::BuildChannelInfo& GlobalState::BuildChannel() {
  return build_channel_;
}

mojom::SysInfo& GlobalState::SysInfo() {
  return sys_info_;
}

}  // namespace brave_ads
