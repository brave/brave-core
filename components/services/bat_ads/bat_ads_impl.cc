/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <functional>

#include "base/check.h"
#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_content_value_util.h"
#include "bat/ads/ads.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_filter_types.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/history_item_value_util.h"
#include "bat/ads/history_sort_types.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/inline_content_ad_value_util.h"
#include "bat/ads/new_tab_page_ad_info.h"
#include "bat/ads/new_tab_page_ad_value_util.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/notification_ad_value_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace bat_ads {

namespace {

ads::CategoryContentOptActionType ToCategoryContentOptActionType(
    const int opt_action_type) {
  return static_cast<ads::CategoryContentOptActionType>(opt_action_type);
}

}  // namespace

BatAdsImpl::BatAdsImpl(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client)
    : bat_ads_client_mojo_proxy_(new BatAdsClientMojoBridge(std::move(client))),
      ads_(ads::Ads::CreateInstance(bat_ads_client_mojo_proxy_.get())) {}

BatAdsImpl::~BatAdsImpl() = default;

void BatAdsImpl::Initialize(
    InitializeCallback callback) {
  auto* holder = new CallbackHolder<InitializeCallback>(AsWeakPtr(),
      std::move(callback));

  ads_->Initialize(std::bind(BatAdsImpl::OnInitialize, holder, _1));
}

void BatAdsImpl::Shutdown(
    ShutdownCallback callback) {
  auto* holder = new CallbackHolder<ShutdownCallback>(AsWeakPtr(),
      std::move(callback));

  auto shutdown_callback = std::bind(BatAdsImpl::OnShutdown, holder, _1);
  ads_->Shutdown(shutdown_callback);
}

void BatAdsImpl::OnLocaleDidChange(const std::string& locale) {
  ads_->OnLocaleDidChange(locale);
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

void BatAdsImpl::OnUserGesture(const int32_t page_transition_type) {
  ads_->OnUserGesture(page_transition_type);
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

void BatAdsImpl::OnMediaPlaying(
    const int32_t tab_id) {
  ads_->OnMediaPlaying(tab_id);
}

void BatAdsImpl::OnMediaStopped(
    const int32_t tab_id) {
  ads_->OnMediaStopped(tab_id);
}

void BatAdsImpl::OnTabUpdated(const int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const bool is_active,
                              const bool is_browser_active,
                              const bool is_incognito) {
  ads_->OnTabUpdated(tab_id, redirect_chain, is_active, is_browser_active,
                     is_incognito);
}

void BatAdsImpl::OnTabClosed(
    const int32_t tab_id) {
  ads_->OnTabClosed(tab_id);
}

void BatAdsImpl::MaybeGetNotificationAd(
    const std::string& placement_id,
    MaybeGetNotificationAdCallback callback) {
  const absl::optional<ads::NotificationAdInfo> ad =
      ads_->MaybeGetNotificationAd(placement_id);
  if (!ad) {
    std::move(callback).Run(/*ad*/ absl::nullopt);
    return;
  }

  absl::optional<base::Value::Dict> dict = ads::NotificationAdToValue(*ad);
  std::move(callback).Run(std::move(dict));
}

void BatAdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const ads::mojom::NotificationAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  ads_->TriggerNotificationAdEvent(placement_id, event_type);
}

void BatAdsImpl::MaybeServeNewTabPageAd(
    MaybeServeNewTabPageAdCallback callback) {
  auto* holder = new CallbackHolder<MaybeServeNewTabPageAdCallback>(
      AsWeakPtr(), std::move(callback));

  auto maybe_serve_new_tab_page_ad_callback =
      std::bind(BatAdsImpl::OnMaybeServeNewTabPageAd, holder, _1);
  ads_->MaybeServeNewTabPageAd(maybe_serve_new_tab_page_ad_callback);
}

void BatAdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::NewTabPageAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  ads_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                 event_type);
}

void BatAdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::PromotedContentAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  ads_->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                      event_type);
}

void BatAdsImpl::MaybeServeInlineContentAd(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  auto* holder = new CallbackHolder<MaybeServeInlineContentAdCallback>(
      AsWeakPtr(), std::move(callback));

  auto maybe_serve_inline_content_ads_callback =
      std::bind(BatAdsImpl::OnMaybeServeInlineContentAd, holder, _1, _2);
  ads_->MaybeServeInlineContentAd(dimensions,
                                  maybe_serve_inline_content_ads_callback);
}

void BatAdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::InlineContentAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  ads_->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                    event_type);
}

void BatAdsImpl::TriggerSearchResultAdEvent(
    ads::mojom::SearchResultAdInfoPtr ad_mojom,
    const ads::mojom::SearchResultAdEventType event_type) {
  DCHECK(ads::mojom::IsKnownEnumValue(event_type));

  ads_->TriggerSearchResultAdEvent(std::move(ad_mojom), event_type);
}

void BatAdsImpl::PurgeOrphanedAdEventsForType(
    const ads::mojom::AdType ad_type,
    PurgeOrphanedAdEventsForTypeCallback callback) {
  DCHECK(ads::mojom::IsKnownEnumValue(ad_type));

  auto* holder = new CallbackHolder<PurgeOrphanedAdEventsForTypeCallback>(
      AsWeakPtr(), std::move(callback));

  auto purge_ad_events_for_type_callback =
      std::bind(BatAdsImpl::OnPurgeOrphanedAdEventsForType, holder, _1);

  ads_->PurgeOrphanedAdEventsForType(ad_type,
                                     purge_ad_events_for_type_callback);
}

void BatAdsImpl::RemoveAllHistory(
    RemoveAllHistoryCallback callback) {
  auto* holder = new CallbackHolder<RemoveAllHistoryCallback>(AsWeakPtr(),
      std::move(callback));

  auto remove_all_history_callback =
      std::bind(BatAdsImpl::OnRemoveAllHistory, holder, _1);
  ads_->RemoveAllHistory(remove_all_history_callback);
}

void BatAdsImpl::OnWalletUpdated(
    const std::string& payment_id,
    const std::string& seed) {
  ads_->OnWalletUpdated(payment_id, seed);
}

void BatAdsImpl::GetHistory(const base::Time from_time,
                            const base::Time to_time,
                            GetHistoryCallback callback) {
  const ads::HistoryItemList history_items = ads_->GetHistory(
      ads::HistoryFilterType::kConfirmationType,
      ads::HistorySortType::kDescendingOrder, from_time, to_time);

  std::move(callback).Run(ads::HistoryItemsToUIValue(history_items));
}

void BatAdsImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  auto* holder = new CallbackHolder<GetStatementOfAccountsCallback>(
      AsWeakPtr(), std::move(callback));

  ads_->GetStatementOfAccounts(
      std::bind(BatAdsImpl::OnGetStatementOfAccounts, holder, _1));
}

void BatAdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  auto* holder = new CallbackHolder<GetDiagnosticsCallback>(
      AsWeakPtr(), std::move(callback));

  ads_->GetDiagnostics(std::bind(BatAdsImpl::OnGetDiagnostics, holder, _1));
}

void BatAdsImpl::ToggleAdThumbUp(base::Value::Dict value,
                                 ToggleAdThumbUpCallback callback) {
  ads::AdContentInfo ad_content = ads::AdContentFromValue(value);
  ad_content.like_action_type = ads_->ToggleAdThumbUp(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleAdThumbDown(base::Value::Dict value,
                                   ToggleAdThumbDownCallback callback) {
  ads::AdContentInfo ad_content = ads::AdContentFromValue(value);
  ad_content.like_action_type = ads_->ToggleAdThumbDown(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleAdOptIn(const std::string& category,
                               const int opt_action_type,
                               ToggleAdOptInCallback callback) {
  const ads::CategoryContentOptActionType toggled_opt_action_type =
      ads_->ToggleAdOptIn(category,
                          ToCategoryContentOptActionType(opt_action_type));
  std::move(callback).Run(category, static_cast<int>(toggled_opt_action_type));
}

void BatAdsImpl::ToggleAdOptOut(const std::string& category,
                                const int opt_action_type,
                                ToggleAdOptOutCallback callback) {
  const ads::CategoryContentOptActionType toggled_opt_action_type =
      ads_->ToggleAdOptOut(category,
                           ToCategoryContentOptActionType(opt_action_type));
  std::move(callback).Run(category, static_cast<int>(toggled_opt_action_type));
}

void BatAdsImpl::ToggleSavedAd(base::Value::Dict value,
                               ToggleSavedAdCallback callback) {
  ads::AdContentInfo ad_content = ads::AdContentFromValue(value);
  ad_content.is_saved = ads_->ToggleSavedAd(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::ToggleFlaggedAd(base::Value::Dict value,
                                 ToggleFlaggedAdCallback callback) {
  ads::AdContentInfo ad_content = ads::AdContentFromValue(value);
  ad_content.is_flagged = ads_->ToggleFlaggedAd(std::move(value));
  std::move(callback).Run(AdContentToValue(ad_content));
}

void BatAdsImpl::OnDidUpdateResourceComponent(const std::string& id) {
  ads_->OnDidUpdateResourceComponent(id);
}

///////////////////////////////////////////////////////////////////////////////

void BatAdsImpl::OnInitialize(CallbackHolder<InitializeCallback>* holder,
                              const bool success) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success);
  }

  delete holder;
}

void BatAdsImpl::OnShutdown(CallbackHolder<ShutdownCallback>* holder,
                            const bool success) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success);
  }

  delete holder;
}

// static
void BatAdsImpl::OnMaybeServeNewTabPageAd(
    CallbackHolder<MaybeServeNewTabPageAdCallback>* holder,
    const absl::optional<ads::NewTabPageAdInfo>& ad) {
  DCHECK(holder);
  if (holder->is_valid()) {
    if (!ad) {
      std::move(holder->get()).Run(/*ad*/ absl::nullopt);
      return;
    }

    absl::optional<base::Value::Dict> dict = ads::NewTabPageAdToValue(*ad);
    std::move(holder->get()).Run(std::move(dict));
  }

  delete holder;
}

void BatAdsImpl::OnMaybeServeInlineContentAd(
    CallbackHolder<MaybeServeInlineContentAdCallback>* holder,
    const std::string& dimensions,
    const absl::optional<ads::InlineContentAdInfo>& ad) {
  if (holder->is_valid()) {
    if (!ad) {
      std::move(holder->get()).Run(dimensions, /*ads*/ absl::nullopt);
      return;
    }

    absl::optional<base::Value::Dict> dict = ads::InlineContentAdToValue(*ad);
    std::move(holder->get()).Run(dimensions, std::move(dict));
  }

  delete holder;
}

void BatAdsImpl::OnPurgeOrphanedAdEventsForType(
    CallbackHolder<PurgeOrphanedAdEventsForTypeCallback>* holder,
    const bool success) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success);
  }

  delete holder;
}

void BatAdsImpl::OnRemoveAllHistory(
    CallbackHolder<RemoveAllHistoryCallback>* holder,
    const bool success) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success);
  }

  delete holder;
}

void BatAdsImpl::OnGetStatementOfAccounts(
    CallbackHolder<GetStatementOfAccountsCallback>* holder,
    ads::mojom::StatementInfoPtr statement) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(statement));
  }

  delete holder;
}

// static
void BatAdsImpl::OnGetDiagnostics(
    CallbackHolder<GetDiagnosticsCallback>* holder,
    absl::optional<base::Value::List> value) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(value));
  }

  delete holder;
}

}  // namespace bat_ads
