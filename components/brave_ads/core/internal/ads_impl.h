/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/ads_service_callback.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct NotificationAdInfo;

namespace database {
class Maintenance;
}  // namespace database

class AdsImpl final : public Ads {
 public:
  AdsImpl(AdsClient* ads_client,
          std::unique_ptr<TokenGeneratorInterface> token_generator);

  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  AdsImpl(AdsImpl&&) noexcept = delete;
  AdsImpl& operator=(AdsImpl&&) noexcept = delete;

  ~AdsImpl() override;

  // Ads:
  void AddBatAdsObserver(
      std::unique_ptr<AdsObserverInterface> observer) override;

  void SetSysInfo(mojom::SysInfoPtr mojom_sys_info) override;
  void SetBuildChannel(mojom::BuildChannelInfoPtr mojom_build_channel) override;
  void SetFlags(mojom::FlagsPtr mojom_flags) override;

  void Initialize(mojom::WalletInfoPtr mojom_wallet,
                  InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  std::optional<NotificationAdInfo> MaybeGetNotificationAd(
      const std::string& placement_id) override;
  void TriggerNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(
      mojom::AdType mojom_ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  void GetAdHistory(base::Time from_time,
                    base::Time to_time,
                    GetAdHistoryForUICallback callback) override;

  void ToggleLikeAd(const base::Value::Dict& value,
                    ToggleReactionCallback callback) override;
  void ToggleDislikeAd(const base::Value::Dict& value,
                       ToggleReactionCallback callback) override;
  void ToggleLikeSegment(const base::Value::Dict& value,
                         ToggleReactionCallback callback) override;
  void ToggleDislikeSegment(const base::Value::Dict& value,
                            ToggleReactionCallback callback) override;
  void ToggleSaveAd(const base::Value::Dict& value,
                    ToggleReactionCallback callback) override;
  void ToggleMarkAdAsInappropriate(const base::Value::Dict& value,
                                   ToggleReactionCallback callback) override;

 private:
  void CreateOrOpenDatabase(mojom::WalletInfoPtr mojom_wallet,
                            InitializeCallback callback);
  void CreateOrOpenDatabaseCallback(mojom::WalletInfoPtr mojom_wallet,
                                    InitializeCallback callback,
                                    bool success);
  void SuccessfullyInitialized(mojom::WalletInfoPtr mojom_wallet,
                               InitializeCallback callback);

  void PurgeAllOrphanedAdEventsCallback(mojom::WalletInfoPtr mojom_wallet,
                                        InitializeCallback callback,
                                        bool success);

  // TODO(https://github.com/brave/brave-browser/issues/39795): Transition away
  // from using JSON state to a more efficient data approach.
  void MigrateClientStateCallback(mojom::WalletInfoPtr mojom_wallet,
                                  InitializeCallback callback,
                                  bool success);
  void LoadClientStateCallback(mojom::WalletInfoPtr mojom_wallet,
                               InitializeCallback callback,
                               bool success);
  void MigrateConfirmationStateCallback(mojom::WalletInfoPtr mojom_wallet,
                                        InitializeCallback callback,
                                        bool success);
  void LoadConfirmationStateCallback(mojom::WalletInfoPtr mojom_wallet,
                                     InitializeCallback callback,
                                     bool success);

  bool is_initialized_ = false;

  // TODO(https://github.com/brave/brave-browser/issues/37622): Deprecate global
  // state.
  GlobalState global_state_;

  // Handles database maintenance tasks, such as purging and vacuuming.
  std::unique_ptr<database::Maintenance> database_maintenance_;

  base::WeakPtrFactory<AdsImpl> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
