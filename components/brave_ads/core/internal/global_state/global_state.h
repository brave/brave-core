/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"

namespace brave_ads {

class AdsClient;
class GlobalStateHolder;
class BrowserManager;
class ClientStateManager;
class ConfirmationStateManager;
class DatabaseManager;
class DiagnosticManager;
class FlagManager;
class HistoryManager;
class IdleDetection;
class NotificationAdManager;
class PredictorsManager;
class TabManager;
class UserActivityManager;

class GlobalState final {
 public:
  explicit GlobalState(AdsClient* ads_client);

  GlobalState(const GlobalState& other) = delete;
  GlobalState& operator=(const GlobalState& other) = delete;

  GlobalState(GlobalState&& other) noexcept = delete;
  GlobalState& operator=(GlobalState&& other) noexcept = delete;

  ~GlobalState();

  static GlobalState* GetInstance();

  static bool HasInstance();

  AdsClient* GetAdsClient();

  BrowserManager* GetBrowserManager();

  ClientStateManager* GetClientStateManager();

  ConfirmationStateManager* GetConfirmationStateManager();

  DatabaseManager* GetDatabaseManager();

  DiagnosticManager* GetDiagnosticManager();

  FlagManager* GetFlagManager();

  HistoryManager* GetHistoryManager();

  NotificationAdManager* GetNotificationAdManager();

  PredictorsManager* GetPredictorsManager();

  TabManager* GetTabManager();

  UserActivityManager* GetUserActivityManager();

  mojom::BuildChannelInfo& BuildChannel();

  mojom::SysInfo& SysInfo();

 private:
  raw_ptr<AdsClient> ads_client_ = nullptr;

  std::unique_ptr<GlobalStateHolder> global_state_holder_;

  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<ClientStateManager> client_state_manager_;
  std::unique_ptr<ConfirmationStateManager> confirmation_state_manager_;
  std::unique_ptr<DatabaseManager> database_manager_;
  std::unique_ptr<DiagnosticManager> diagnostic_manager_;
  std::unique_ptr<FlagManager> flag_manager_;
  std::unique_ptr<HistoryManager> history_manager_;
  std::unique_ptr<IdleDetection> idle_detection_;
  std::unique_ptr<NotificationAdManager> notification_ad_manager_;
  std::unique_ptr<PredictorsManager> predictors_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<UserActivityManager> user_activity_manager_;

  mojom::BuildChannelInfo build_channel_;
  mojom::SysInfo sys_info_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
