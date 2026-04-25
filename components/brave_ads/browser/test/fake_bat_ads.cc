/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_bat_ads.h"

#include <optional>
#include <utility>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

FakeBatAds::FakeBatAds(base::RepeatingClosure initialize_callback,
                       bool simulate_initialization_failure)
    : initialize_callback_(std::move(initialize_callback)),
      simulate_initialization_failure_(simulate_initialization_failure) {}

FakeBatAds::~FakeBatAds() = default;

void FakeBatAds::BindReceiver(
    mojo::PendingAssociatedReceiver<bat_ads::mojom::BatAds>
        bat_ads_pending_associated_receiver) {
  bat_ads_associated_receiver_.Bind(
      std::move(bat_ads_pending_associated_receiver));
}

void FakeBatAds::Initialize(brave_ads::mojom::WalletInfoPtr /*mojom_wallet*/,
                            InitializeCallback callback) {
  if (!simulate_initialization_failure_) {
    initialize_callback_.Run();
  }
  std::move(callback).Run(!simulate_initialization_failure_);
}

void FakeBatAds::Shutdown(ShutdownCallback callback) {
  std::move(callback).Run(/*success=*/true);
}

void FakeBatAds::GetInternals(GetInternalsCallback callback) {
  std::move(callback).Run(/*value=*/std::nullopt);
}

void FakeBatAds::GetDiagnostics(GetDiagnosticsCallback callback) {
  std::move(callback).Run(/*value=*/std::nullopt);
}

void FakeBatAds::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  std::move(callback).Run(/*mojom_statement=*/nullptr);
}

void FakeBatAds::ParseAndSaveNewTabPageAds(
    base::DictValue /*value*/,
    ParseAndSaveNewTabPageAdsCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  std::move(callback).Run(/*value=*/nullptr);
}

void FakeBatAds::TriggerNewTabPageAdEvent(
    const std::string& /*placement_id*/,
    const std::string& /*creative_instance_id*/,
    brave_ads::mojom::NewTabPageAdMetricType /*mojom_ad_metric_type*/,
    brave_ads::mojom::NewTabPageAdEventType /*mojom_ad_event_type*/,
    TriggerNewTabPageAdEventCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::MaybeGetNotificationAd(
    const std::string& /*placement_id*/,
    MaybeGetNotificationAdCallback callback) {
  std::move(callback).Run(/*value=*/std::nullopt);
}

void FakeBatAds::TriggerNotificationAdEvent(
    const std::string& /*placement_id*/,
    brave_ads::mojom::NotificationAdEventType /*mojom_ad_event_type*/,
    TriggerNotificationAdEventCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::MaybeGetSearchResultAd(
    const std::string& /*placement_id*/,
    MaybeGetSearchResultAdCallback callback) {
  std::move(callback).Run(/*search_result_ad=*/nullptr);
}

void FakeBatAds::TriggerSearchResultAdEvent(
    brave_ads::mojom::CreativeSearchResultAdInfoPtr /*mojom_creative_ad*/,
    brave_ads::mojom::SearchResultAdEventType /*mojom_ad_event_type*/,
    TriggerSearchResultAdEventCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::PurgeOrphanedAdEventsForType(
    brave_ads::mojom::AdType /*mojom_ad_type*/,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::GetAdHistory(base::Time /*from_time*/,
                              base::Time /*to_time*/,
                              GetAdHistoryCallback callback) {
  std::move(callback).Run(/*value=*/std::nullopt);
}

void FakeBatAds::ToggleLikeAd(
    brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
    ToggleLikeAdCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::ToggleDislikeAd(
    brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
    ToggleDislikeAdCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::ToggleLikeSegment(
    brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
    ToggleLikeSegmentCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::ToggleDislikeSegment(
    brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
    ToggleDislikeSegmentCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::ToggleSaveAd(
    brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
    ToggleSaveAdCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

void FakeBatAds::ToggleMarkAdAsInappropriate(
    brave_ads::mojom::ReactionInfoPtr /*mojom_reaction*/,
    ToggleMarkAdAsInappropriateCallback callback) {
  std::move(callback).Run(/*success=*/false);
}

}  // namespace brave_ads::test
