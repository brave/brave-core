/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/fl/predictors/predictors_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state_holder.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_manager.h"

namespace brave_ads {

class AdsClient;
class GlobalStateHolder;

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

  BrowserManager& GetBrowserManager();
  ClientStateManager& GetClientStateManager();
  ConfirmationStateManager& GetConfirmationStateManager();
  DatabaseManager& GetDatabaseManager();
  DiagnosticManager& GetDiagnosticManager();
  HistoryManager& GetHistoryManager();
  NotificationAdManager& GetNotificationAdManager();
  PredictorsManager& GetPredictorsManager();
  TabManager& GetTabManager();
  UserActivityManager& GetUserActivityManager();

  mojom::SysInfo& SysInfo();

  mojom::BuildChannelInfo& BuildChannel();

  mojom::Flags& Flags();

 private:
  const raw_ptr<AdsClient> ads_client_ = nullptr;

  const std::unique_ptr<GlobalStateHolder> global_state_holder_;

  BrowserManager browser_manager_;
  ClientStateManager client_state_manager_;
  ConfirmationStateManager confirmation_state_manager_;
  DatabaseManager database_manager_;
  DiagnosticManager diagnostic_manager_;
  HistoryManager history_manager_;
  IdleDetection idle_detection_;
  NotificationAdManager notification_ad_manager_;
  PredictorsManager predictors_manager_;
  TabManager tab_manager_;
  UserActivityManager user_activity_manager_;

  mojom::SysInfo sys_info_;
  mojom::BuildChannelInfo build_channel_;
  mojom::Flags flags_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
