/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

// TODO(https://github.com/brave/brave-browser/issues/37622): Deprecate global
// state.

class AdHistoryManager;
class AdsClient;
class AdsCore;
class AdsNotifierManager;
class BrowserManager;
class ClientStateManager;
class ConfirmationStateManager;
class DatabaseManager;
class DiagnosticManager;
class GlobalStateHolder;
class NotificationAdManager;
class TabManager;
class TokenGeneratorInterface;
class UserActivityManager;

class GlobalState final {
 public:
  GlobalState(AdsClient& ads_client,
              const base::FilePath& database_path,
              std::unique_ptr<TokenGeneratorInterface> token_generator);

  GlobalState(const GlobalState& other) = delete;
  GlobalState& operator=(const GlobalState& other) = delete;

  GlobalState(GlobalState&& other) noexcept = delete;
  GlobalState& operator=(GlobalState&& other) noexcept = delete;

  ~GlobalState();

  static GlobalState* GetInstance();

  static bool HasInstance();

  AdsClient& GetAdsClient();

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
  AdsCore& GetAdsCore();

  mojom::SysInfo& SysInfo();
  mojom::BuildChannelInfo& BuildChannel();
  mojom::Flags& Flags();

  void PostDelayedTask(base::OnceClosure task, base::TimeDelta delay);

 private:
  void PostDelayedTaskCallback(base::OnceClosure task);

  SEQUENCE_CHECKER(sequence_checker_);

  const raw_ref<AdsClient> ads_client_;

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
  std::unique_ptr<AdsCore> ads_core_;

  mojom::SysInfo mojom_sys_info_;
  mojom::BuildChannelInfo mojom_build_channel_;
  mojom::Flags mojom_flags_;

  base::WeakPtrFactory<GlobalState> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_H_
