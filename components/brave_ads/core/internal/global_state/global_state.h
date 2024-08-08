/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

// TODO(https://github.com/brave/brave-browser/issues/37622): Deprecate global
// state.

class AdHistoryManager;
class AdsClient;
class AdsNotifierManager;
class BrowserManager;
class ClientStateManager;
class ConfirmationStateManager;
class DatabaseManager;
class DiagnosticManager;
class GlobalStateHolder;
class NotificationAdManager;
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

  AdsNotifierManager& GetAdsNotifierManager();
  BrowserManager& GetBrowserManager();
  ClientStateManager& GetClientStateManager();
  ConfirmationStateManager& GetConfirmationStateManager();
  DatabaseManager& GetDatabaseManager();
  DiagnosticManager& GetDiagnosticManager();
  AdHistoryManager& GetHistoryManager();
  NotificationAdManager& GetNotificationAdManager();
  TabManager& GetTabManager();
  UserActivityManager& GetUserActivityManager();

  mojom::SysInfo& SysInfo();
  mojom::BuildChannelInfo& BuildChannel();
  mojom::Flags& Flags();

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  const raw_ptr<AdsClient> ads_client_ = nullptr;  // NOT OWNED

  const std::unique_ptr<GlobalStateHolder> global_state_holder_;

  std::unique_ptr<AdsNotifierManager> ads_notifier_manager_;
  std::unique_ptr<BrowserManager> browser_manager_;
  std::unique_ptr<ClientStateManager> client_state_manager_;
  std::unique_ptr<ConfirmationStateManager> confirmation_state_manager_;
  std::unique_ptr<DatabaseManager> database_manager_;
  std::unique_ptr<DiagnosticManager> diagnostic_manager_;
  std::unique_ptr<AdHistoryManager> ad_history_manager_;
  std::unique_ptr<NotificationAdManager> notification_ad_manager_;
  std::unique_ptr<TabManager> tab_manager_;
  std::unique_ptr<UserActivityManager> user_activity_manager_;

  mojom::SysInfo sys_info_;
  mojom::BuildChannelInfo build_channel_;
  mojom::Flags flags_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
