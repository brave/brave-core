/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/ads_client.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state_holder.h"

namespace brave_ads {

GlobalState::GlobalState(AdsClient* ads_client)
    : ads_client_(ads_client),
      global_state_holder_(std::make_unique<GlobalStateHolder>(this)) {
  DCHECK(ads_client_);
}

GlobalState::~GlobalState() = default;

// static
GlobalState* GlobalState::GetInstance() {
  DCHECK(GlobalStateHolder::GetGlobalState());
  return GlobalStateHolder::GetGlobalState();
}

// static
bool GlobalState::HasInstance() {
  return GlobalStateHolder::GetGlobalState() != nullptr;
}

AdsClient* GlobalState::GetAdsClient() {
  DCHECK(ads_client_);
  return ads_client_;
}

BrowserManager& GlobalState::GetBrowserManager() {
  return browser_manager_;
}

ClientStateManager& GlobalState::GetClientStateManager() {
  return client_state_manager_;
}

ConfirmationStateManager& GlobalState::GetConfirmationStateManager() {
  return confirmation_state_manager_;
}

DatabaseManager& GlobalState::GetDatabaseManager() {
  return database_manager_;
}

DiagnosticManager& GlobalState::GetDiagnosticManager() {
  return diagnostic_manager_;
}

HistoryManager& GlobalState::GetHistoryManager() {
  return history_manager_;
}

NotificationAdManager& GlobalState::GetNotificationAdManager() {
  return notification_ad_manager_;
}

PredictorsManager& GlobalState::GetPredictorsManager() {
  return predictors_manager_;
}

TabManager& GlobalState::GetTabManager() {
  return tab_manager_;
}

UserActivityManager& GlobalState::GetUserActivityManager() {
  return user_activity_manager_;
}

mojom::SysInfo& GlobalState::SysInfo() {
  return sys_info_;
}

mojom::BuildChannelInfo& GlobalState::BuildChannel() {
  return build_channel_;
}

mojom::Flags& GlobalState::Flags() {
  return flags_;
}

}  // namespace brave_ads
