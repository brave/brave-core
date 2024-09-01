/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core.h"
#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state_holder.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

GlobalState::GlobalState(
    AdsClient* const ads_client,
    std::unique_ptr<TokenGeneratorInterface> token_generator)
    : ads_client_(ads_client),
      global_state_holder_(std::make_unique<GlobalStateHolder>(this)) {
  CHECK(ads_client_);

  ads_notifier_manager_ = std::make_unique<AdsNotifierManager>();
  browser_manager_ = std::make_unique<BrowserManager>();
  client_state_manager_ = std::make_unique<ClientStateManager>();
  confirmation_state_manager_ = std::make_unique<ConfirmationStateManager>();
  database_manager_ = std::make_unique<DatabaseManager>();
  diagnostic_manager_ = std::make_unique<DiagnosticManager>();
  ad_history_manager_ = std::make_unique<AdHistoryManager>();
  notification_ad_manager_ = std::make_unique<NotificationAdManager>();
  tab_manager_ = std::make_unique<TabManager>();
  user_activity_manager_ = std::make_unique<UserActivityManager>();
  ads_core_ = std::make_unique<AdsCore>(std::move(token_generator));
}

GlobalState::~GlobalState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
GlobalState* GlobalState::GetInstance() {
  CHECK(GlobalStateHolder::GetGlobalState());

  return GlobalStateHolder::GetGlobalState();
}

// static
bool GlobalState::HasInstance() {
  return GlobalStateHolder::GetGlobalState() != nullptr;
}

AdsClient* GlobalState::GetAdsClient() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(ads_client_);
  return ads_client_;
}

AdsNotifierManager& GlobalState::GetAdsNotifierManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(ads_notifier_manager_);
  return *ads_notifier_manager_;
}

BrowserManager& GlobalState::GetBrowserManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(browser_manager_);
  return *browser_manager_;
}

ClientStateManager& GlobalState::GetClientStateManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(client_state_manager_);
  return *client_state_manager_;
}

ConfirmationStateManager& GlobalState::GetConfirmationStateManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(confirmation_state_manager_);
  return *confirmation_state_manager_;
}

DatabaseManager& GlobalState::GetDatabaseManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(database_manager_);
  return *database_manager_;
}

DiagnosticManager& GlobalState::GetDiagnosticManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(diagnostic_manager_);
  return *diagnostic_manager_;
}

AdHistoryManager& GlobalState::GetHistoryManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(ad_history_manager_);
  return *ad_history_manager_;
}

NotificationAdManager& GlobalState::GetNotificationAdManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(notification_ad_manager_);
  return *notification_ad_manager_;
}

TabManager& GlobalState::GetTabManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(tab_manager_);
  return *tab_manager_;
}

UserActivityManager& GlobalState::GetUserActivityManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(user_activity_manager_);
  return *user_activity_manager_;
}

AdsCore& GlobalState::GetAdsCore() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(ads_core_);
  return *ads_core_;
}

mojom::SysInfo& GlobalState::SysInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return mojom_sys_info_;
}

mojom::BuildChannelInfo& GlobalState::BuildChannel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return mojom_build_channel_;
}

mojom::Flags& GlobalState::Flags() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return mojom_flags_;
}

}  // namespace brave_ads
