/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"
#include "brave/components/brave_ads/core/public/ads_observer.h"
#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"
#include "brave/components/services/bat_ads/bat_ads_observer.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace bat_ads {

class BatAdsImpl::AdsInstance final {
 public:
  AdsInstance(
      const base::FilePath& service_path,
      mojo::PendingAssociatedRemote<mojom::BatAdsClient>
          bat_ads_client_pending_associated_remote,
      mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier)
      : bat_ads_client_mojo_proxy_(std::make_unique<BatAdsClientMojoBridge>(
            std::move(bat_ads_client_pending_associated_remote),
            std::move(client_notifier))),
        ads_(brave_ads::Ads::CreateInstance(
            *bat_ads_client_mojo_proxy_,
            service_path.AppendASCII(brave_ads::kDatabaseFilename))) {}

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

BatAdsImpl::BatAdsImpl(const base::FilePath& service_path,
                       mojo::PendingAssociatedRemote<mojom::BatAdsClient>
                           bat_ads_client_pending_associated_remote,
                       mojo::PendingReceiver<mojom::BatAdsClientNotifier>
                           bat_ads_client_notifier_pending_receiver)
    : ads_instance_(std::unique_ptr<AdsInstance, base::OnTaskRunnerDeleter>(
          new AdsInstance(service_path,
                          std::move(bat_ads_client_pending_associated_remote),
                          std::move(bat_ads_client_notifier_pending_receiver)),
          base::OnTaskRunnerDeleter(
              base::SequencedTaskRunner::GetCurrentDefault()))) {}

BatAdsImpl::~BatAdsImpl() = default;

void BatAdsImpl::AddBatAdsObserver(mojo::PendingRemote<mojom::BatAdsObserver>
                                       bat_ads_observer_pending_remote) {
  std::unique_ptr<brave_ads::AdsObserver> ads_observer =
      std::make_unique<BatAdsObserver>(
          std::move(bat_ads_observer_pending_remote));
  GetAds()->AddObserver(std::move(ads_observer));
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

void BatAdsImpl::SetContentSettings(
    brave_ads::mojom::ContentSettingsPtr mojom_content_settings) {
  GetAds()->SetContentSettings(std::move(mojom_content_settings));
}

void BatAdsImpl::Initialize(brave_ads::mojom::WalletInfoPtr mojom_wallet,
                            InitializeCallback callback) {
  LOG(ERROR) << "FOOBAR.Initialize";
  GetAds()->Initialize(std::move(mojom_wallet),
                       mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                           std::move(callback), /*success=*/false));
}

void BatAdsImpl::Shutdown(ShutdownCallback callback) {
  LOG(ERROR) << "FOOBAR.Shutdown";
  GetAds()->Shutdown(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), /*success=*/false));
}

void BatAdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id,
    MaybeGetNotificationAdCallback callback) {
  GetAds()->MaybeGetNotificationAd(
      placement_id,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(
              [](MaybeGetNotificationAdCallback callback,
                 base::optional_ref<const brave_ads::NotificationAdInfo> ad) {
                if (!ad) {
                  return std::move(callback).Run(/*ad*/ std::nullopt);
                }

                std::optional<base::Value::Dict> dict =
                    brave_ads::NotificationAdToValue(*ad);
                std::move(callback).Run(std::move(dict));
              },
              std::move(callback)),
          /*value=*/std::nullopt));
}

void BatAdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    brave_ads::mojom::NotificationAdEventType mojom_ad_event_type,
    TriggerNotificationAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerNotificationAdEvent(
      placement_id, mojom_ad_event_type,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success=*/false));
}

void BatAdsImpl::ParseAndSaveNewTabPageAds(
    base::Value::Dict data,
    ParseAndSaveNewTabPageAdsCallback callback) {
  LOG(ERROR) << "FOOBAR.ParseAndSaveNewTabPageAds."
                "WrapCallbackWithDefaultInvokeIfNotRun";
  GetAds()->ParseAndSaveNewTabPageAds(
      std::move(data), mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                           std::move(callback), /*success=*/false));
}

void BatAdsImpl::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  LOG(ERROR) << "FOOBAR.MaybeServeNewTabPageAd";
  GetAds()->MaybeServeNewTabPageAd(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      base::BindOnce(
          [](MaybeServeNewTabPageAdCallback callback,
             base::optional_ref<const brave_ads::NewTabPageAdInfo> ad) {
            if (!ad) {
              std::move(callback).Run(/*ad*/ std::nullopt);
              return;
            }

            std::optional<base::Value::Dict> dict =
                brave_ads::NewTabPageAdToValue(*ad);
            std::move(callback).Run(std::move(dict));
          },
          std::move(callback)),
      /*value=*/std::nullopt));
}

void BatAdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type,
    brave_ads::mojom::NewTabPageAdEventType mojom_ad_event_type,
    TriggerNewTabPageAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  LOG(ERROR) << "FOOBAR.TriggerNewTabPageAdEvent";
  GetAds()->TriggerNewTabPageAdEvent(
      placement_id, creative_instance_id, mojom_ad_metric_type,
      mojom_ad_event_type,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success=*/false));
}

void BatAdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::PromotedContentAdEventType mojom_ad_event_type,
    TriggerPromotedContentAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerPromotedContentAdEvent(
      placement_id, creative_instance_id, mojom_ad_event_type,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success=*/false));
}

void BatAdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  GetAds()->MaybeServeInlineContentAd(
      dimensions,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(
              [](MaybeServeInlineContentAdCallback callback,
                 const std::string& dimensions,
                 base::optional_ref<const brave_ads::InlineContentAdInfo> ad) {
                if (!ad) {
                  std::move(callback).Run(dimensions,
                                          /*ads*/ std::nullopt);
                  return;
                }

                std::optional<base::Value::Dict> dict =
                    brave_ads::InlineContentAdToValue(*ad);
                std::move(callback).Run(dimensions, std::move(dict));
              },
              std::move(callback)),
          /*dimensions=*/std::string(),
          /*value=*/std::nullopt));
}

void BatAdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    brave_ads::mojom::InlineContentAdEventType mojom_ad_event_type,
    TriggerInlineContentAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerInlineContentAdEvent(
      placement_id, creative_instance_id, mojom_ad_event_type,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success=*/false));
}

void BatAdsImpl::MaybeGetSearchResultAd(
    const std::string& placement_id,
    MaybeGetSearchResultAdCallback callback) {
  GetAds()->MaybeGetSearchResultAd(
      placement_id, mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                        std::move(callback), /*search_result_ad=*/brave_ads::
                            mojom::CreativeSearchResultAdInfoPtr()));
}

void BatAdsImpl::TriggerSearchResultAdEvent(
    brave_ads::mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
    brave_ads::mojom::SearchResultAdEventType mojom_ad_event_type,
    TriggerSearchResultAdEventCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_event_type));

  GetAds()->TriggerSearchResultAdEvent(
      std::move(mojom_creative_ad), mojom_ad_event_type,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success=*/false));
}

void BatAdsImpl::PurgeOrphanedAdEventsForType(
    brave_ads::mojom::AdType mojom_ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  CHECK(brave_ads::mojom::IsKnownEnumValue(mojom_ad_type));

  LOG(ERROR) << "FOOBAR.PurgeOrphanedAdEventsForType: " << mojom_ad_type;
  GetAds()->PurgeOrphanedAdEventsForType(
      mojom_ad_type,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*success=*/false));
}

void BatAdsImpl::GetAdHistory(base::Time from_time,
                              base::Time to_time,
                              GetAdHistoryCallback callback) {
  GetAds()->GetAdHistory(from_time, to_time,
                         mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                             std::move(callback), /*value=*/std::nullopt));
}

void BatAdsImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  GetAds()->GetStatementOfAccounts(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), /*mojom_statement=*/nullptr));
}

void BatAdsImpl::GetInternals(GetInternalsCallback callback) {
  GetAds()->GetInternals(
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                  /*value=*/std::nullopt));
}

void BatAdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  GetAds()->GetDiagnostics(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), /*value=*/std::nullopt));
}

void BatAdsImpl::ToggleLikeAd(brave_ads::mojom::ReactionInfoPtr reaction,
                              ToggleLikeAdCallback callback) {
  GetAds()->ToggleLikeAd(std::move(reaction),
                         mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                             std::move(callback), /*success=*/false));
}

void BatAdsImpl::ToggleDislikeAd(brave_ads::mojom::ReactionInfoPtr reaction,
                                 ToggleDislikeAdCallback callback) {
  GetAds()->ToggleDislikeAd(std::move(reaction),
                            mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                                std::move(callback), /*success=*/false));
}

void BatAdsImpl::ToggleLikeSegment(brave_ads::mojom::ReactionInfoPtr reaction,
                                   ToggleLikeSegmentCallback callback) {
  GetAds()->ToggleLikeSegment(std::move(reaction),
                              mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                                  std::move(callback), /*success=*/false));
}

void BatAdsImpl::ToggleDislikeSegment(
    brave_ads::mojom::ReactionInfoPtr reaction,
    ToggleDislikeSegmentCallback callback) {
  GetAds()->ToggleDislikeSegment(std::move(reaction),
                                 mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                                     std::move(callback), /*success=*/false));
}

void BatAdsImpl::ToggleSaveAd(brave_ads::mojom::ReactionInfoPtr reaction,
                              ToggleSaveAdCallback callback) {
  GetAds()->ToggleSaveAd(std::move(reaction),
                         mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                             std::move(callback), /*success=*/false));
}

void BatAdsImpl::ToggleMarkAdAsInappropriate(
    brave_ads::mojom::ReactionInfoPtr reaction,
    ToggleMarkAdAsInappropriateCallback callback) {
  GetAds()->ToggleMarkAdAsInappropriate(
      std::move(reaction), mojo::WrapCallbackWithDefaultInvokeIfNotRun(
                               std::move(callback), /*success=*/false));
}

brave_ads::Ads* BatAdsImpl::GetAds() {
  CHECK(ads_instance_);

  CHECK(ads_instance_->GetAds());
  return ads_instance_->GetAds();
}

}  // namespace bat_ads
