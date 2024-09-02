/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_observer_interface.h"
#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"
#include "brave/components/services/bat_ads/bat_ads_observer.h"

namespace bat_ads {

class BatAdsImpl::AdsInstance final {
 public:
  AdsInstance(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient>
          bat_ads_client_pending_associated_remote,
      mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier)
      : bat_ads_client_mojo_proxy_(std::make_unique<BatAdsClientMojoBridge>(
            std::move(bat_ads_client_pending_associated_remote),
            std::move(client_notifier))),
        ads_(brave_ads::Ads::CreateInstance(bat_ads_client_mojo_proxy_.get())) {
  }

  AdsInstance(const AdsInstance&) = delete;
  AdsInstance& operator=(const AdsInstance&) = delete;

  AdsInstance(AdsInstance&& other) noexcept = delete;
  AdsInstance& operator=(AdsInstance&& other) noexcept = delete;

  ~AdsInstance() = default;

  brave_ads::Ads* GetAds() { return ads_.get(); }

 private:
  std::unique_ptr<BatAdsClientMojoBridge> bat_ads_client_mojo_proxy_;
  std::unique_ptr<brave_ads::Ads> ads_;
};

BatAdsImpl::BatAdsImpl(mojo::PendingAssociatedRemote<mojom::BatAdsClient>
                           bat_ads_client_pending_associated_remote,
                       mojo::PendingReceiver<mojom::BatAdsClientNotifier>
                           bat_ads_client_notifier_pending_receiver)
    : ads_instance_(std::unique_ptr<AdsInstance, base::OnTaskRunnerDeleter>(
          new AdsInstance(std::move(bat_ads_client_pending_associated_remote),
                          std::move(bat_ads_client_notifier_pending_receiver)),
          base::OnTaskRunnerDeleter(
              base::SequencedTaskRunner::GetCurrentDefault()))) {}

BatAdsImpl::~BatAdsImpl() = default;

void BatAdsImpl::AddBatAdsObserver(mojo::PendingRemote<mojom::BatAdsObserver>
                                       bat_ads_observer_pending_remote) {
  std::unique_ptr<brave_ads::AdsObserverInterface> ads_observer =
      std::make_unique<BatAdsObserver>(
          std::move(bat_ads_observer_pending_remote));
  GetAds()->AddBatAdsObserver(std::move(ads_observer));
}

void BatAdsImpl::SetSysInfo(brave_ads::mojom::SysInfoPtr mojom_sys_info) {
  GetAds()->SetSysInfo(std::move(mojom_sys_info));
}

void BatAdsImpl::SetBuildChannel(
    brave_ads::mojom::BuildChannelInfoPtr mojom_build_channel) {
  GetAds()->SetBuildChannel(std::move(mojom_build_channel));
}

void BatAdsImpl::SetFlags(brave_ads::mojom::FlagsPtr mojom_flags) {
  GetAds()->SetFlags(std::move(mojom_flags));
}

void BatAdsImpl::Initialize(brave_ads::mojom::WalletInfoPtr mojom_wallet,
                            InitializeCallback callback) {
  GetAds()->Initialize(std::move(mojom_wallet), std::move(callback));
}

void BatAdsImpl::Shutdown(ShutdownCallback callback) {
  GetAds()->Shutdown(std::move(callback));
}

void BatAdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id,
    MaybeGetNotificationAdCallback callback) {
  const std::optional<brave_ads::NotificationAdInfo> ad =
      GetAds()->MaybeGetNotificationAd(placement_id);
  if (!ad) {
    std::move(callback).Run(/*ad*/ std::nullopt);
    return;
  }

  std::optional<base::Value::Dict> dict = brave_ads::NotificationAdToValue(*ad);
  std::move(callback).Run(std::move(dict));
}

void BatAdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const brave_ads::mojom::NotificationAdEventType mojom_ad_event_type,
    TriggerNotificationAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerNotificationAdEvent(placement_id, mojom_ad_event_type,
                                       std::move(callback));
}

void BatAdsImpl::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  GetAds()->MaybeServeNewTabPageAd(base::BindOnce(
      [](MaybeServeNewTabPageAdCallback callback,
         const std::optional<brave_ads::NewTabPageAdInfo>& ad) {
        if (!ad) {
          std::move(callback).Run(/*ad*/ std::nullopt);
          return;
        }

        std::optional<base::Value::Dict> dict =
            brave_ads::NewTabPageAdToValue(*ad);
        std::move(callback).Run(std::move(dict));
      },
      std::move(callback)));
}

void BatAdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
    TriggerNewTabPageAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                     mojom_ad_event_type, std::move(callback));
}

void BatAdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const brave_ads::mojom::PromotedContentAdEventType mojom_ad_event_type,
    TriggerPromotedContentAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                          mojom_ad_event_type,
                                          std::move(callback));
}

void BatAdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  GetAds()->MaybeServeInlineContentAd(
      dimensions,
      base::BindOnce(
          [](MaybeServeInlineContentAdCallback callback,
             const std::string& dimensions,
             const std::optional<brave_ads::InlineContentAdInfo>& ad) {
            if (!ad) {
              std::move(callback).Run(dimensions,
                                      /*ads*/ std::nullopt);
              return;
            }

            std::optional<base::Value::Dict> dict =
                brave_ads::InlineContentAdToValue(*ad);
            std::move(callback).Run(dimensions, std::move(dict));
          },
          std::move(callback)));
}

void BatAdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const brave_ads::mojom::InlineContentAdEventType mojom_ad_event_type,
    TriggerInlineContentAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                        mojom_ad_event_type,
                                        std::move(callback));
}

void BatAdsImpl::TriggerSearchResultAdEvent(
    brave_ads::mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    const brave_ads::mojom::SearchResultAdEventType mojom_ad_event_type,
    TriggerSearchResultAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerSearchResultAdEvent(
      std::move(mojom_creative_ad), mojom_ad_event_type, std::move(callback));
}

void BatAdsImpl::PurgeOrphanedAdEventsForType(
    const brave_ads::mojom::AdType mojom_ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_type));

  GetAds()->PurgeOrphanedAdEventsForType(mojom_ad_type, std::move(callback));
}

void BatAdsImpl::GetAdHistory(const base::Time from_time,
                              const base::Time to_time,
                              GetAdHistoryCallback callback) {
  GetAds()->GetAdHistory(from_time, to_time, std::move(callback));
}

void BatAdsImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  GetAds()->GetStatementOfAccounts(std::move(callback));
}

void BatAdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  GetAds()->GetDiagnostics(std::move(callback));
}

void BatAdsImpl::ToggleLikeAd(base::Value::Dict value,
                              ToggleLikeAdCallback callback) {
  GetAds()->ToggleLikeAd(value, std::move(callback));
}

void BatAdsImpl::ToggleDislikeAd(base::Value::Dict value,
                                 ToggleDislikeAdCallback callback) {
  GetAds()->ToggleDislikeAd(value, std::move(callback));
}

void BatAdsImpl::ToggleLikeSegment(base::Value::Dict value,
                                   ToggleLikeSegmentCallback callback) {
  GetAds()->ToggleLikeSegment(value, std::move(callback));
}

void BatAdsImpl::ToggleDislikeSegment(base::Value::Dict value,
                                      ToggleDislikeSegmentCallback callback) {
  GetAds()->ToggleDislikeSegment(value, std::move(callback));
}

void BatAdsImpl::ToggleSaveAd(base::Value::Dict value,
                              ToggleSaveAdCallback callback) {
  GetAds()->ToggleSaveAd(value, std::move(callback));
}

void BatAdsImpl::ToggleMarkAdAsInappropriate(
    base::Value::Dict value,
    ToggleMarkAdAsInappropriateCallback callback) {
  GetAds()->ToggleMarkAdAsInappropriate(value, std::move(callback));
}

brave_ads::Ads* BatAdsImpl::GetAds() {
  CHECK(ads_instance_);

  CHECK(ads_instance_->GetAds());
  return ads_instance_->GetAds();
}

}  // namespace bat_ads
