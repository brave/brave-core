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

  void SetSysInfo(mojom::SysInfoPtr sys_info) override;
  void SetBuildChannel(mojom::BuildChannelInfoPtr build_channel) override;
  void SetFlags(mojom::FlagsPtr flags) override;

  void Initialize(mojom::WalletInfoPtr wallet,
                  InitializeCallback callback) override;
  void Shutdown(ShutdownCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(const std::string& placement_id,
                                   const std::string& creative_instance_id,
                                   mojom::InlineContentAdEventType event_type,
                                   TriggerAdEventCallback callback) override;

  void MaybeServeNewTabPageAd(MaybeServeNewTabPageAdCallback callback) override;
  void TriggerNewTabPageAdEvent(const std::string& placement_id,
                                const std::string& creative_instance_id,
                                mojom::NewTabPageAdEventType event_type,
                                TriggerAdEventCallback callback) override;

  std::optional<NotificationAdInfo> MaybeGetNotificationAd(
      const std::string& placement_id) override;
  void TriggerNotificationAdEvent(const std::string& placement_id,
                                  mojom::NotificationAdEventType event_type,
                                  TriggerAdEventCallback callback) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType event_type,
      TriggerAdEventCallback callback) override;

  void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType event_type,
      TriggerAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(
      mojom::AdType ad_type,
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
  void CreateOrOpenDatabase(mojom::WalletInfoPtr wallet,
                            InitializeCallback callback);
  void CreateOrOpenDatabaseCallback(mojom::WalletInfoPtr wallet,
                                    InitializeCallback callback,
                                    bool success);
  void SuccessfullyInitialized(mojom::WalletInfoPtr wallet,
                               InitializeCallback callback);

  // TODO(https://github.com/brave/brave-browser/issues/40265): Periodically
  // purge expired and orphaned state.
  void PurgeExpiredAdEventsCallback(mojom::WalletInfoPtr wallet,
                                    InitializeCallback callback,
                                    bool success);
  void PurgeAllOrphanedAdEventsCallback(mojom::WalletInfoPtr wallet,
                                        InitializeCallback callback,
                                        bool success);

  // TODO(https://github.com/brave/brave-browser/issues/39795): Transition away
  // from using JSON state to a more efficient data approach.
  void MigrateClientStateCallback(mojom::WalletInfoPtr wallet,
                                  InitializeCallback callback,
                                  bool success);
  void LoadClientStateCallback(mojom::WalletInfoPtr wallet,
                               InitializeCallback callback,
                               bool success);
  void MigrateConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                        InitializeCallback callback,
                                        bool success);
  void LoadConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                     InitializeCallback callback,
                                     bool success);

  bool is_initialized_ = false;

  // TODO(https://github.com/brave/brave-browser/issues/37622): Deprecate global
  // state.
  GlobalState global_state_;

  base::WeakPtrFactory<AdsImpl> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_IMPL_H_
