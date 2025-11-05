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
#include "brave/components/brave_ads/core/browser/service/new_tab_page_ad_prefetcher.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/flags/flags_util.h"
#include "components/prefs/pref_service.h"
#include "sql/database.h"

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
      new_tab_page_ad_prefetcher_(
          std::make_unique<NewTabPageAdPrefetcher>(/*ads_service=*/*this)) {
  CHECK(prefs_);
}

AdsServiceImplIOS::~AdsServiceImplIOS() = default;

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
    return std::move(callback).Run(/*success=*/false);
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
  prefs_->ClearPrefsWithPrefixSilently("brave.brave_ads");

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

void AdsServiceImplIOS::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(dimensions,
                                   /*inline_content_ad*/ std::nullopt);
  }

  ads_->MaybeServeInlineContentAd(dimensions, std::move(callback));
}

void AdsServiceImplIOS::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    mojom::InlineContentAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_event_type));

  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                    mojom_ad_event_type, std::move(callback));
}

void AdsServiceImplIOS::PrefetchNewTabPageAd() {
  if (!IsInitialized()) {
    return;
  }

  new_tab_page_ad_prefetcher_->Prefetch();
}

std::optional<NewTabPageAdInfo>
AdsServiceImplIOS::MaybeGetPrefetchedNewTabPageAd() {
  if (!IsInitialized()) {
    return std::nullopt;
  }

  return new_tab_page_ad_prefetcher_->MaybeGetPrefetchedAd();
}

void AdsServiceImplIOS::OnFailedToPrefetchNewTabPageAd(
    const std::string& /*placement_id*/,
    const std::string& /*creative_instance_id*/) {
  ResetNewTabPageAd();

  PurgeOrphanedAdEventsForType(mojom::AdType::kNewTabPageAd,
                               /*intentional*/ base::DoNothing());
}

void AdsServiceImplIOS::ParseAndSaveNewTabPageAds(
    base::Value::Dict dict,
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
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->ParseAndSaveNewTabPageAds(
      std::move(dict),
      base::BindOnce(&AdsServiceImplIOS::OnParseAndSaveNewTabPageAdsCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsServiceImplIOS::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  if (!IsInitialized()) {
    return std::move(callback).Run(/*ad=*/std::nullopt);
  }

  ads_->MaybeServeNewTabPageAd(std::move(callback));
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

void AdsServiceImplIOS::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    mojom::PromotedContentAdEventType mojom_ad_event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(mojom_ad_event_type));

  if (!IsInitialized()) {
    return std::move(callback).Run(/*success*/ false);
  }

  ads_->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                      mojom_ad_event_type, std::move(callback));
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
    int32_t /*tab_id*/,
    const std::vector<GURL>& /*redirect_chain*/,
    const std::string& /*text*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyTabHtmlContentDidChange(
    int32_t /*tab_id*/,
    const std::vector<GURL>& /*redirect_chain*/,
    const std::string& /*html*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyTabDidStartPlayingMedia(int32_t /*tab_id*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyTabDidStopPlayingMedia(int32_t /*tab_id*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyTabDidChange(
    int32_t /*tab_id*/,
    const std::vector<GURL>& /*redirect_chain*/,
    bool /*is_new_navigation*/,
    bool /*is_restoring*/,
    bool /*is_visible*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyTabDidLoad(int32_t /*tab_id*/,
                                         int /*http_status_code*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyDidCloseTab(int32_t /*tab_id*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyUserGestureEventTriggered(
    int32_t /*page_transition_type*/) {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyBrowserDidBecomeActive() {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyBrowserDidResignActive() {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

void AdsServiceImplIOS::NotifyDidSolveAdaptiveCaptcha() {
  // TODO(https://github.com/brave/brave-browser/issues/42373): Utilize
  // AdsClientNotifier in AdsServiceImplIOS
  NOTIMPLEMENTED() << "Not used on iOS.";
}

///////////////////////////////////////////////////////////////////////////////

void AdsServiceImplIOS::Shutdown() {
  ResetNewTabPageAd();

  NotifyDidShutdownAdsService();

  ads_.reset();
}

void AdsServiceImplIOS::InitializeAds(InitializeCallback callback) {
  CHECK(!IsInitialized());

  ads_ = Ads::CreateInstance(*ads_client_,
                             storage_path_.AppendASCII(kAdsDatabaseFilename));

  ads_->SetSysInfo(mojom_sys_info_.Clone());
  ads_->SetBuildChannel(mojom_build_channel_.Clone());
  ads_->SetFlags(BuildFlags());

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

  // Ensure the Brave Ads service is stopped before clearing data.
  if (IsInitialized()) {
    return std::move(callback).Run(/*success=*/false);
  }

  file_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(
          [](const base::FilePath& storage_path) {
            sql::Database::Delete(storage_path.Append(kAdsDatabaseFilename));

            base::DeleteFile(storage_path.Append(kClientJsonFilename));

            base::DeleteFile(storage_path.Append(kConfirmationsJsonFilename));
          },
          storage_path_),
      base::BindOnce(&AdsServiceImplIOS::ClearAdsDataCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AdsServiceImplIOS::ClearAdsDataCallback(ClearDataCallback callback) {
  NotifyDidClearAdsServiceData();
  InitializeAds(std::move(callback));
}

void AdsServiceImplIOS::RefetchNewTabPageAd() {
  ResetNewTabPageAd();

  PurgeOrphanedAdEventsForType(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&AdsServiceImplIOS::RefetchNewTabPageAdCallback,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AdsServiceImplIOS::RefetchNewTabPageAdCallback(bool success) {
  if (success) {
    new_tab_page_ad_prefetcher_->Prefetch();
  }
}

void AdsServiceImplIOS::ResetNewTabPageAd() {
  new_tab_page_ad_prefetcher_ =
      std::make_unique<NewTabPageAdPrefetcher>(/*ads_service=*/*this);
}

void AdsServiceImplIOS::OnParseAndSaveNewTabPageAdsCallback(
    ParseAndSaveNewTabPageAdsCallback callback,
    bool success) {
  if (success) {
    PrefetchNewTabPageAd();
  }

  std::move(callback).Run(success);
}

}  // namespace brave_ads
