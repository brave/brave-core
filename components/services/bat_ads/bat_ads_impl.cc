/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/ad_content_info.h"
#include "brave/components/brave_ads/core/ad_content_value_util.h"
#include "brave/components/brave_ads/core/ads.h"
#include "brave/components/brave_ads/core/history_filter_types.h"
#include "brave/components/brave_ads/core/history_item_info.h"
#include "brave/components/brave_ads/core/history_item_value_util.h"
#include "brave/components/brave_ads/core/history_sort_types.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"
#include "brave/components/brave_ads/core/inline_content_ad_value_util.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_value_util.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "brave/components/brave_ads/core/notification_ad_value_util.h"
#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace bat_ads {

class BatAdsImpl::AdsInstance final {
 public:
  AdsInstance(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client,
      mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier)
      : bat_ads_client_mojo_proxy_(std::make_unique<BatAdsClientMojoBridge>(
            std::move(client),
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

BatAdsImpl::BatAdsImpl(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client,
    mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier)
    : ads_instance_(std::unique_ptr<AdsInstance, base::OnTaskRunnerDeleter>(
          new AdsInstance(std::move(client), std::move(client_notifier)),
          base::OnTaskRunnerDeleter(
              base::SequencedTaskRunner::GetCurrentDefault()))) {}

BatAdsImpl::~BatAdsImpl() = default;

void BatAdsImpl::SetSysInfo(brave_ads::mojom::SysInfoPtr sys_info) {
  GetAds()->SetSysInfo(std::move(sys_info));
}

void BatAdsImpl::SetBuildChannel(
    brave_ads::mojom::BuildChannelInfoPtr build_channel) {
  GetAds()->SetBuildChannel(std::move(build_channel));
}

void BatAdsImpl::SetFlags(brave_ads::mojom::FlagsPtr flags) {
  GetAds()->SetFlags(std::move(flags));
}

void BatAdsImpl::Initialize(InitializeCallback callback) {
  GetAds()->Initialize(std::move(callback));
}

void BatAdsImpl::Shutdown(ShutdownCallback callback) {
  GetAds()->Shutdown(std::move(callback));
}

void BatAdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id,
    MaybeGetNotificationAdCallback callback) {
  const absl::optional<brave_ads::NotificationAdInfo> ad =
      GetAds()->MaybeGetNotificationAd(placement_id);
  if (!ad) {
    std::move(callback).Run(/*ad*/ absl::nullopt);
    return;
  }

  absl::optional<base::Value::Dict> dict =
      brave_ads::NotificationAdToValue(*ad);
  std::move(callback).Run(std::move(dict));
}

void BatAdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const brave_ads::mojom::NotificationAdEventType event_type) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(event_type));

  GetAds()->TriggerNotificationAdEvent(placement_id, event_type);
}

void BatAdsImpl::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  GetAds()->MaybeServeNewTabPageAd(base::BindOnce(
      [](MaybeServeNewTabPageAdCallback callback,
         const absl::optional<brave_ads::NewTabPageAdInfo>& ad) {
        if (!ad) {
          std::move(callback).Run(/*ad*/ absl::nullopt);
          return;
        }

        absl::optional<base::Value::Dict> dict =
            brave_ads::NewTabPageAdToValue(*ad);
        std::move(callback).Run(std::move(dict));
      },
      std::move(callback)));
}

void BatAdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const brave_ads::mojom::NewTabPageAdEventType event_type) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(event_type));

  GetAds()->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                     event_type);
}

void BatAdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const brave_ads::mojom::PromotedContentAdEventType event_type) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(event_type));

  GetAds()->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                          event_type);
}

void BatAdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  GetAds()->MaybeServeInlineContentAd(
      dimensions,
      base::BindOnce(
          [](MaybeServeInlineContentAdCallback callback,
             const std::string& dimensions,
             const absl::optional<brave_ads::InlineContentAdInfo>& ad) {
            if (!ad) {
              std::move(callback).Run(dimensions,
                                      /*ads*/ absl::nullopt);
              return;
            }

            absl::optional<base::Value::Dict> dict =
                brave_ads::InlineContentAdToValue(*ad);
            std::move(callback).Run(dimensions, std::move(dict));
          },
          std::move(callback)));
}

void BatAdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const brave_ads::mojom::InlineContentAdEventType event_type) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(event_type));

  GetAds()->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                        event_type);
}

void BatAdsImpl::TriggerSearchResultAdEvent(
    brave_ads::mojom::SearchResultAdInfoPtr ad_mojom,
    const brave_ads::mojom::SearchResultAdEventType event_type) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(event_type));

  GetAds()->TriggerSearchResultAdEvent(std::move(ad_mojom), event_type);
}

void BatAdsImpl::PurgeOrphanedAdEventsForType(
    const brave_ads::mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(ad_type));

  GetAds()->PurgeOrphanedAdEventsForType(ad_type, std::move(callback));
}

void BatAdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  GetAds()->RemoveAllHistory(std::move(callback));
}

void BatAdsImpl::OnRewardsWalletDidChange(const std::string& payment_id,
                                          const std::string& recovery_seed) {
  GetAds()->OnRewardsWalletDidChange(payment_id, recovery_seed);
}

void BatAdsImpl::GetHistory(const base::Time from_time,
                            const base::Time to_time,
                            GetHistoryCallback callback) {
  const brave_ads::HistoryItemList history_items = GetAds()->GetHistory(
      brave_ads::HistoryFilterType::kConfirmationType,
      brave_ads::HistorySortType::kDescendingOrder, from_time, to_time);

  std::move(callback).Run(brave_ads::HistoryItemsToUIValue(history_items));
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
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.user_reaction_type = GetAds()->ToggleLikeAd(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleDislikeAd(base::Value::Dict value,
                                 ToggleDislikeAdCallback callback) {
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.user_reaction_type = GetAds()->ToggleDislikeAd(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleLikeCategory(
    const std::string& category,
    const brave_ads::mojom::UserReactionType user_reaction_type,
    ToggleLikeCategoryCallback callback) {
  const brave_ads::mojom::UserReactionType toggled_user_reaction_type =
      GetAds()->ToggleLikeCategory(category, user_reaction_type);
  std::move(callback).Run(category, toggled_user_reaction_type);
}

void BatAdsImpl::ToggleDislikeCategory(
    const std::string& category,
    const brave_ads::mojom::UserReactionType user_reaction_type,
    ToggleDislikeCategoryCallback callback) {
  const brave_ads::mojom::UserReactionType toggled_user_reaction_type =
      GetAds()->ToggleDislikeCategory(category, user_reaction_type);
  std::move(callback).Run(category, toggled_user_reaction_type);
}

void BatAdsImpl::ToggleSaveAd(base::Value::Dict value,
                              ToggleSaveAdCallback callback) {
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.is_saved = GetAds()->ToggleSaveAd(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleMarkAdAsInappropriate(
    base::Value::Dict value,
    ToggleMarkAdAsInappropriateCallback callback) {
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.is_flagged =
      GetAds()->ToggleMarkAdAsInappropriate(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

brave_ads::Ads* BatAdsImpl::GetAds() {
  DCHECK(ads_instance_);
  DCHECK(ads_instance_->GetAds());
  return ads_instance_->GetAds();
}

}  // namespace bat_ads
