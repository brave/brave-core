/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"  // IWYU pragma: keep
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
#include "url/gurl.h"

namespace bat_ads {

namespace {

brave_ads::CategoryContentOptActionType ToCategoryContentOptActionType(
    const int opt_action_type) {
  return static_cast<brave_ads::CategoryContentOptActionType>(opt_action_type);
}

}  // namespace

BatAdsImpl::BatAdsImpl(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client,
    mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier)
    : bat_ads_client_mojo_proxy_(
          new BatAdsClientMojoBridge(std::move(client),
                                     std::move(client_notifier))),
      ads_(brave_ads::Ads::CreateInstance(bat_ads_client_mojo_proxy_.get())) {}

BatAdsImpl::~BatAdsImpl() = default;

void BatAdsImpl::Initialize(InitializeCallback callback) {
  ads_->Initialize(std::move(callback));
}

void BatAdsImpl::Shutdown(ShutdownCallback callback) {
  ads_->Shutdown(std::move(callback));
}

void BatAdsImpl::OnPrefDidChange(const std::string& path) {
  ads_->OnPrefDidChange(path);
}

void BatAdsImpl::OnTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  ads_->OnTabHtmlContentDidChange(tab_id, redirect_chain, html);
}

void BatAdsImpl::OnTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  ads_->OnTabTextContentDidChange(tab_id, redirect_chain, text);
}

void BatAdsImpl::TriggerUserGestureEvent(const int32_t page_transition_type) {
  ads_->TriggerUserGestureEvent(page_transition_type);
}

void BatAdsImpl::OnUserDidBecomeActive(const base::TimeDelta idle_time,
                                       const bool screen_was_locked) {
  ads_->OnUserDidBecomeActive(idle_time, screen_was_locked);
}

void BatAdsImpl::OnUserDidBecomeIdle() {
  ads_->OnUserDidBecomeIdle();
}

void BatAdsImpl::OnBrowserDidEnterForeground() {
  ads_->OnBrowserDidEnterForeground();
}

void BatAdsImpl::OnBrowserDidEnterBackground() {
  ads_->OnBrowserDidEnterBackground();
}

void BatAdsImpl::OnTabDidStartPlayingMedia(const int32_t tab_id) {
  ads_->OnTabDidStartPlayingMedia(tab_id);
}

void BatAdsImpl::OnTabDidStopPlayingMedia(const int32_t tab_id) {
  ads_->OnTabDidStopPlayingMedia(tab_id);
}

void BatAdsImpl::OnTabDidChange(const int32_t tab_id,
                                const std::vector<GURL>& redirect_chain,
                                const bool is_active,
                                const bool is_browser_active,
                                const bool is_incognito) {
  ads_->OnTabDidChange(tab_id, redirect_chain, is_active, is_browser_active,
                       is_incognito);
}

void BatAdsImpl::OnDidCloseTab(const int32_t tab_id) {
  ads_->OnDidCloseTab(tab_id);
}

void BatAdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id,
    MaybeGetNotificationAdCallback callback) {
  const absl::optional<brave_ads::NotificationAdInfo> ad =
      ads_->MaybeGetNotificationAd(placement_id);
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

  ads_->TriggerNotificationAdEvent(placement_id, event_type);
}

void BatAdsImpl::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  ads_->MaybeServeNewTabPageAd(base::BindOnce(
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

  ads_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                 event_type);
}

void BatAdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const brave_ads::mojom::PromotedContentAdEventType event_type) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(event_type));

  ads_->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                      event_type);
}

void BatAdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  ads_->MaybeServeInlineContentAd(
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

  ads_->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                    event_type);
}

void BatAdsImpl::TriggerSearchResultAdEvent(
    brave_ads::mojom::SearchResultAdInfoPtr ad_mojom,
    const brave_ads::mojom::SearchResultAdEventType event_type) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(event_type));

  ads_->TriggerSearchResultAdEvent(std::move(ad_mojom), event_type);
}

void BatAdsImpl::PurgeOrphanedAdEventsForType(
    const brave_ads::mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  DCHECK(brave_ads::mojom::IsKnownEnumValue(ad_type));

  ads_->PurgeOrphanedAdEventsForType(ad_type, std::move(callback));
}

void BatAdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  ads_->RemoveAllHistory(std::move(callback));
}

void BatAdsImpl::OnRewardsWalletDidChange(const std::string& payment_id,
                                          const std::string& recovery_seed) {
  ads_->OnRewardsWalletDidChange(payment_id, recovery_seed);
}

void BatAdsImpl::GetHistory(const base::Time from_time,
                            const base::Time to_time,
                            GetHistoryCallback callback) {
  const brave_ads::HistoryItemList history_items = ads_->GetHistory(
      brave_ads::HistoryFilterType::kConfirmationType,
      brave_ads::HistorySortType::kDescendingOrder, from_time, to_time);

  std::move(callback).Run(brave_ads::HistoryItemsToUIValue(history_items));
}

void BatAdsImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  ads_->GetStatementOfAccounts(std::move(callback));
}

void BatAdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  ads_->GetDiagnostics(std::move(callback));
}

void BatAdsImpl::ToggleAdThumbUp(base::Value::Dict value,
                                 ToggleAdThumbUpCallback callback) {
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.like_action_type = ads_->ToggleAdThumbUp(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleAdThumbDown(base::Value::Dict value,
                                   ToggleAdThumbDownCallback callback) {
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.like_action_type = ads_->ToggleAdThumbDown(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleAdOptIn(const std::string& category,
                               const int opt_action_type,
                               ToggleAdOptInCallback callback) {
  const brave_ads::CategoryContentOptActionType toggled_opt_action_type =
      ads_->ToggleAdOptIn(category,
                          ToCategoryContentOptActionType(opt_action_type));
  std::move(callback).Run(category, static_cast<int>(toggled_opt_action_type));
}

void BatAdsImpl::ToggleAdOptOut(const std::string& category,
                                const int opt_action_type,
                                ToggleAdOptOutCallback callback) {
  const brave_ads::CategoryContentOptActionType toggled_opt_action_type =
      ads_->ToggleAdOptOut(category,
                           ToCategoryContentOptActionType(opt_action_type));
  std::move(callback).Run(category, static_cast<int>(toggled_opt_action_type));
}

void BatAdsImpl::ToggleSavedAd(base::Value::Dict value,
                               ToggleSavedAdCallback callback) {
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.is_saved = ads_->ToggleSavedAd(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleFlaggedAd(base::Value::Dict value,
                                 ToggleFlaggedAdCallback callback) {
  brave_ads::AdContentInfo ad_content = brave_ads::AdContentFromValue(value);
  ad_content.is_flagged = ads_->ToggleFlaggedAd(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::OnDidUpdateResourceComponent(const std::string& id) {
  ads_->OnDidUpdateResourceComponent(id);
}

}  // namespace bat_ads
