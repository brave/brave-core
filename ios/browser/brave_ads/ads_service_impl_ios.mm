/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "base/notimplemented.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_util.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/command_line_switches/command_line_switches_util.h"
#include "components/prefs/pref_service.h"
#include "sql/database.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr char kClearDataHistogramName[] = "Brave.Ads.ClearData";
constexpr char kAdsDatabaseFilename[] = "Ads.db";

}  // namespace

AdsServiceImplIOS::AdsServiceImplIOS(PrefService* prefs)
    : AdsService(/*delegate=*/nullptr),
      prefs_(prefs),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ads_client_notifier_(std::make_unique<AdsClientNotifier>()) {
  CHECK(prefs_);
}

AdsServiceImplIOS::~AdsServiceImplIOS() = default;

AdsClientNotifier* AdsServiceImplIOS::GetAdsClientNotifier() {
  return ads_client_notifier_.get();
}

bool AdsServiceImplIOS::IsInitialized() const {
  return !!ads_;
}

void AdsServiceImplIOS::InitializeAds(
    const std::string& storage_path,
    std::unique_ptr<AdsClient> ads_client,
    mojom::SysInfoPtr mojom_sys_info,
    mojom::BuildChannelInfoPtr mojom_build_channel,
    mojom::WalletInfoPtr mojom_wallet,
    InitializeCallback callback) {
  if (IsInitialized()) {
    return std::move(callback).Run(/*success=*/false);
  }

  storage_path_ = base::FilePath(storage_path);
  ads_client_ = std::move(ads_client);
  mojom_sys_info_ = std::move(mojom_sys_info);
  mojom_build_channel_ = std::move(mojom_build_channel);
  mojom_wallet_ = std::move(mojom_wallet);

  InitializeAds(std::move(callback));
}

void AdsServiceImplIOS::ShutdownAds(ShutdownCallback callback) {
  if (!IsInitialized()) {
    // Not initialized means already shut down, which is a success. Callers such
    // as `ClearData` rely on this to proceed even when the service is idle.
    return std::move(callback).Run(/*success=*/true);
  }

  ads_->Shutdown(base::BindOnce(&AdsServiceImplIOS::ShutdownAdsCallback,
                                weak_ptr_factory_.GetWeakPtr(),
                                std::move(callback)));
}

void AdsServiceImplIOS::MaybeGetNotificationAd(
    const std::string& placement_id,
    MaybeGetNotificationAdCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*ad*/ std::nullopt);
  }

  ads_->MaybeGetNotificationAd(placement_id, std::move(callback));
}

void AdsServiceImplIOS::TriggerNotificationAdEvent(
    const std::string& placement_id,
    mojom::NotificationAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->TriggerNotificationAdEvent(placement_id, mojom_ad_event_type,
                                   std::move(callback));
}

void AdsServiceImplIOS::NotifyDidInitializeAdsService() const {
  for (AdsServiceObserver& observer : observers_) {
    observer.OnDidInitializeAdsService();
  }
}

void AdsServiceImplIOS::NotifyDidShutdownAdsService() const {
  for (AdsServiceObserver& observer : observers_) {
    observer.OnDidShutdownAdsService();
  }
}

void AdsServiceImplIOS::NotifyDidClearAdsServiceData() const {
  for (AdsServiceObserver& observer : observers_) {
    observer.OnDidClearAdsServiceData();
  }
}

bool AdsServiceImplIOS::IsBrowserUpgradeRequiredToServeAds() const {
  return false;
}

int64_t AdsServiceImplIOS::GetMaximumNotificationAdsPerHour() const {
  NOTIMPLEMENTED() << "Not used on iOS.";
  return 0;
}

void AdsServiceImplIOS::OnNotificationAdShown(
    const std::string& /*placement_id*/) {
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::OnNotificationAdClosed(
    const std::string& /*placement_id*/,
    bool /*by_user*/) {
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::OnNotificationAdClicked(
    const std::string& /*placement_id*/) {
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::ClearData(ClearDataCallback callback) {
  UMA_HISTOGRAM_BOOLEAN(kClearDataHistogramName, true);
  ShutdownAds(base::BindOnce(&AdsServiceImplIOS::ClearAdsData,
                             weak_ptr_factory_.GetWeakPtr(),
                             std::move(callback)));
}

void AdsServiceImplIOS::AddBatAdsObserver(
    mojo::PendingRemote<bat_ads::mojom::BatAdsObserver>
    /*bat_ads_observer_pending_remote*/) {
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::GetInternals(GetInternalsCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*internals=*/std::nullopt);
  }

  ads_->GetInternals(std::move(callback));
}

void AdsServiceImplIOS::GetDiagnostics(GetDiagnosticsCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*diagnostics*/ std::nullopt);
  }

  ads_->GetDiagnostics(std::move(callback));
}

void AdsServiceImplIOS::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*statement*/ nullptr);
  }

  ads_->GetStatementOfAccounts(std::move(callback));
}

void AdsServiceImplIOS::ParseAndSaveNewTabPageAds(
    base::DictValue dict,
    ParseAndSaveNewTabPageAdsCallback callback) {
  if (task_queue_.should_queue()) {
    // TODO(https://github.com/brave/brave-browser/issues/44925): Transition
    // task queue to ads service layer. API calls are fired before the ads
    // service is initialized, we will follow up with a cross platform solution.
    return task_queue_.Add(base::BindOnce(
        &AdsServiceImplIOS::ParseAndSaveNewTabPageAds,
        weak_ptr_factory_.GetWeakPtr(), std::move(dict), std::move(callback)));
  }

  if (!IsInitialized()) {
    return std::move(callback).Run(/*success=*/false);
  }

  ads_->ParseAndSaveNewTabPageAds(std::move(dict), std::move(callback));
}

void AdsServiceImplIOS::MaybeServeNewTabPageAd(
    MaybeServeMojomNewTabPageAdCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*ad=*/nullptr);
  }

  ads_->MaybeServeNewTabPageAd(base::BindOnce(
      [](MaybeServeMojomNewTabPageAdCallback callback,
         base::optional_ref<const brave_ads::NewTabPageAdInfo> ad) {
        std::move(callback).Run(brave_ads::ToMojom(ad));
      },
      std::move(callback)));
}

void AdsServiceImplIOS::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    mojom::NewTabPageAdMetricType mojom_ad_metric_type,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_event_type));

  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                 mojom_ad_metric_type, mojom_ad_event_type,
                                 std::move(callback));
}

void AdsServiceImplIOS::MaybeGetSearchResultAd(
    const std::string& placement_id,
    MaybeGetSearchResultAdCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*mojom_creative_ad*/ {});
  }

  ads_->MaybeGetSearchResultAd(placement_id, std::move(callback));
}

void AdsServiceImplIOS::TriggerSearchResultAdEvent(
    mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    mojom::SearchResultAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_event_type));

  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->TriggerSearchResultAdEvent(std::move(mojom_creative_ad),
                                   mojom_ad_event_type, std::move(callback));
}

void AdsServiceImplIOS::PurgeOrphanedAdEventsForType(
    mojom::AdType mojom_ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_type));

  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->PurgeOrphanedAdEventsForType(mojom_ad_type, std::move(callback));
}

void AdsServiceImplIOS::GetAdHistory(base::Time from_time,
                                     base::Time to_time,
                                     GetAdHistoryForUICallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*ad_history*/ std::nullopt);
  }

  ads_->GetAdHistory(from_time, to_time, std::move(callback));
}

void AdsServiceImplIOS::ToggleLikeAd(mojom::ReactionInfoPtr mojom_reaction,
                                     ToggleReactionCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->ToggleLikeAd(std::move(mojom_reaction), std::move(callback));
}

void AdsServiceImplIOS::ToggleDislikeAd(mojom::ReactionInfoPtr mojom_reaction,
                                        ToggleReactionCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->ToggleDislikeAd(std::move(mojom_reaction), std::move(callback));
}

void AdsServiceImplIOS::ToggleLikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                                          ToggleReactionCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->ToggleLikeSegment(std::move(mojom_reaction), std::move(callback));
}

void AdsServiceImplIOS::ToggleDislikeSegment(
    mojom::ReactionInfoPtr mojom_reaction,
    ToggleReactionCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->ToggleDislikeSegment(std::move(mojom_reaction), std::move(callback));
}

void AdsServiceImplIOS::ToggleSaveAd(mojom::ReactionInfoPtr mojom_reaction,
                                     ToggleReactionCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->ToggleSaveAd(std::move(mojom_reaction), std::move(callback));
}

void AdsServiceImplIOS::ToggleMarkAdAsInappropriate(
    mojom::ReactionInfoPtr mojom_reaction,
    ToggleReactionCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->ToggleMarkAdAsInappropriate(std::move(mojom_reaction),
                                    std::move(callback));
}

void AdsServiceImplIOS::NotifyTabTextContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  ads_client_notifier_->NotifyTabTextContentDidChange(tab_id, redirect_chain,
                                                      text);
}

void AdsServiceImplIOS::NotifyTabDidStartPlayingMedia(int32_t tab_id) {
  ads_client_notifier_->NotifyTabDidStartPlayingMedia(tab_id);
}

void AdsServiceImplIOS::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  ads_client_notifier_->NotifyTabDidStopPlayingMedia(tab_id);
}

void AdsServiceImplIOS::NotifyTabDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    bool is_new_navigation,
    bool is_restoring,
    bool is_visible) {
  ads_client_notifier_->NotifyTabDidChange(
      tab_id, redirect_chain, is_new_navigation, is_restoring, is_visible);
}

void AdsServiceImplIOS::NotifyTabDidLoad(int32_t tab_id, int http_status_code) {
  ads_client_notifier_->NotifyTabDidLoad(tab_id, http_status_code);
}

void AdsServiceImplIOS::NotifyDidCloseTab(int32_t tab_id) {
  ads_client_notifier_->NotifyDidCloseTab(tab_id);
}

void AdsServiceImplIOS::NotifyUserGestureEventTriggered(
    int32_t page_transition) {
  ads_client_notifier_->NotifyUserGestureEventTriggered(
      static_cast<ui::PageTransition>(page_transition));
}

void AdsServiceImplIOS::NotifyBrowserDidBecomeActive() {
  ads_client_notifier_->NotifyBrowserDidBecomeActive();
}

void AdsServiceImplIOS::NotifyBrowserDidResignActive() {
  ads_client_notifier_->NotifyBrowserDidResignActive();
}

void AdsServiceImplIOS::NotifyDidSolveAdaptiveCaptcha() {
  ads_client_notifier_->NotifyDidSolveAdaptiveCaptcha();
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImplIOS::Shutdown() {
  NotifyDidShutdownAdsService();

  ads_.reset();
}

void AdsServiceImplIOS::InitializeAds(InitializeCallback callback) {
  CHECK(!IsInitialized());

  ads_ = Ads::CreateInstance(*ads_client_,
                             storage_path_.AppendASCII(kAdsDatabaseFilename));

  ads_->SetSysInfo(mojom_sys_info_.Clone());
  ads_->SetBuildChannel(mojom_build_channel_.Clone());
  ads_->SetCommandLineSwitches(BuildCommandLineSwitches());

  ads_->Initialize(
      mojom_wallet_.Clone(),
      base::BindOnce(&AdsServiceImplIOS::InitializeAdsCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsServiceImplIOS::InitializeAdsCallback(InitializeCallback callback,
                                              bool success) {
  if (!success) {
    Shutdown();
  } else {
    NotifyDidInitializeAdsService();
  }

  task_queue_.FlushAndStopQueueing();

  std::move(callback).Run(success);
}

void AdsServiceImplIOS::ShutdownAdsCallback(ShutdownCallback callback,
                                            bool success) {
  Shutdown();

  std::move(callback).Run(success);
}

void AdsServiceImplIOS::ClearAdsData(ClearDataCallback callback, bool success) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  // `ShutdownAdsCallback` always calls `Shutdown` before invoking this callback
  // with success, so `ads_` must be null by this point.
  CHECK(!IsInitialized());

  // Clear preferences only after confirming shutdown succeeded so the pref
  // state and database are always cleared together or not at all.
  prefs_->ClearPrefsWithPrefixSilently("brave.brave_ads");

  file_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(
          [](const base::FilePath& storage_path) {
            sql::Database::Delete(storage_path.Append(kAdsDatabaseFilename));

            base::DeleteFile(storage_path.Append(kClientJsonFilename));
          },
          storage_path_),
      base::BindOnce(&AdsServiceImplIOS::ClearAdsDataCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsServiceImplIOS::ClearAdsDataCallback(ClearDataCallback callback) {
  NotifyDidClearAdsServiceData();
  InitializeAds(std::move(callback));
}

}  // namespace brave_ads
