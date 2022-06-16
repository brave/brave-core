/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <functional>
#include <vector>

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ads.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_filter_types.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_sort_types.h"
#include "bat/ads/inline_content_ad_info.h"
#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace bat_ads {

namespace {

ads::CategoryContentOptActionType ToCategoryContentOptActionType(
    const int opt_action_type) {
  return static_cast<ads::CategoryContentOptActionType>(opt_action_type);
}

}  // namespace

BatAdsImpl::BatAdsImpl(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info) :
    bat_ads_client_mojo_proxy_(new BatAdsClientMojoBridge(
        std::move(client_info))),
    ads_(ads::Ads::CreateInstance(bat_ads_client_mojo_proxy_.get())) {
}

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

void BatAdsImpl::ChangeLocale(
    const std::string& locale) {
  ads_->ChangeLocale(locale);
}

void BatAdsImpl::OnPrefChanged(const std::string& path) {
  ads_->OnPrefChanged(path);
}

void BatAdsImpl::OnHtmlLoaded(const int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& html) {
  ads_->OnHtmlLoaded(tab_id, redirect_chain, html);
}

void BatAdsImpl::OnTextLoaded(const int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              const std::string& text) {
  ads_->OnTextLoaded(tab_id, redirect_chain, text);
}

void BatAdsImpl::OnUserGesture(const int32_t page_transition_type) {
  ads_->OnUserGesture(page_transition_type);
}

void BatAdsImpl::OnUnIdle(const int idle_time, const bool was_locked) {
  ads_->OnUnIdle(idle_time, was_locked);
}

void BatAdsImpl::OnIdle() {
  ads_->OnIdle();
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
                              const GURL& url,
                              const bool is_active,
                              const bool is_browser_active,
                              const bool is_incognito) {
  ads_->OnTabUpdated(tab_id, url, is_active, is_browser_active, is_incognito);
}

void BatAdsImpl::OnTabClosed(
    const int32_t tab_id) {
  ads_->OnTabClosed(tab_id);
}

void BatAdsImpl::GetNotificationAd(const std::string& placement_id,
                                   GetNotificationAdCallback callback) {
  ads::NotificationAdInfo notification_ad;
  ads_->GetNotificationAd(placement_id, &notification_ad);
  std::move(callback).Run(notification_ad.ToJson());
}

void BatAdsImpl::TriggerNotificationAdEvent(
    const std::string& placement_id,
    const ads::mojom::NotificationAdEventType event_type) {
  ads_->TriggerNotificationAdEvent(placement_id, event_type);
}

void BatAdsImpl::TriggerNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::NewTabPageAdEventType event_type) {
  ads_->TriggerNewTabPageAdEvent(placement_id, creative_instance_id,
                                 event_type);
}

void BatAdsImpl::TriggerPromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::PromotedContentAdEventType event_type) {
  ads_->TriggerPromotedContentAdEvent(placement_id, creative_instance_id,
                                      event_type);
}

void BatAdsImpl::GetInlineContentAd(const std::string& dimensions,
                                    GetInlineContentAdCallback callback) {
  auto* holder = new CallbackHolder<GetInlineContentAdCallback>(
      AsWeakPtr(), std::move(callback));

  auto get_inline_content_ads_callback =
      std::bind(BatAdsImpl::OnGetInlineContentAd, holder, _1, _2, _3);
  ads_->GetInlineContentAd(dimensions, get_inline_content_ads_callback);
}

void BatAdsImpl::TriggerInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const ads::mojom::InlineContentAdEventType event_type) {
  ads_->TriggerInlineContentAdEvent(placement_id, creative_instance_id,
                                    event_type);
}

void BatAdsImpl::TriggerSearchResultAdEvent(
    ads::mojom::SearchResultAdPtr ad_mojom,
    const ads::mojom::SearchResultAdEventType event_type,
    TriggerSearchResultAdEventCallback callback) {
  auto* holder = new CallbackHolder<TriggerSearchResultAdEventCallback>(
      AsWeakPtr(), std::move(callback));

  auto on_search_result_ad_event_callback =
      std::bind(BatAdsImpl::OnTriggerSearchResultAdEvent, holder, _1, _2, _3);
  ads_->TriggerSearchResultAdEvent(std::move(ad_mojom), event_type,
                                   on_search_result_ad_event_callback);
}

void BatAdsImpl::PurgeOrphanedAdEventsForType(
    const ads::mojom::AdType ad_type) {
  ads_->PurgeOrphanedAdEventsForType(ad_type);
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
  ads::HistoryInfo history = ads_->GetHistory(
      ads::HistoryFilterType::kConfirmationType,
      ads::HistorySortType::kDescendingOrder, from_time, to_time);

  std::move(callback).Run(history.ToJson());
}

void BatAdsImpl::GetStatementOfAccounts(
    GetStatementOfAccountsCallback callback) {
  auto* holder = new CallbackHolder<GetStatementOfAccountsCallback>(
      AsWeakPtr(), std::move(callback));

  ads_->GetStatementOfAccounts(
      std::bind(BatAdsImpl::OnGetStatementOfAccounts, holder, _1, _2));
}

void BatAdsImpl::GetDiagnostics(GetDiagnosticsCallback callback) {
  auto* holder = new CallbackHolder<GetDiagnosticsCallback>(
      AsWeakPtr(), std::move(callback));

  ads_->GetDiagnostics(std::bind(BatAdsImpl::OnGetDiagnostics, holder, _1, _2));
}

void BatAdsImpl::ToggleAdThumbUp(const std::string& json,
                                 ToggleAdThumbUpCallback callback) {
  ads::AdContentInfo ad_content;
  ad_content.FromJson(json);
  ad_content.like_action_type = ads_->ToggleAdThumbUp(json);

  std::move(callback).Run(ad_content.ToJson());
}

void BatAdsImpl::ToggleAdThumbDown(const std::string& json,
                                   ToggleAdThumbDownCallback callback) {
  ads::AdContentInfo ad_content;
  ad_content.FromJson(json);
  ad_content.like_action_type = ads_->ToggleAdThumbDown(json);

  std::move(callback).Run(ad_content.ToJson());
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

void BatAdsImpl::ToggleSavedAd(const std::string& json,
                               ToggleSavedAdCallback callback) {
  ads::AdContentInfo ad_content;
  ad_content.FromJson(json);
  ad_content.is_saved = ads_->ToggleSavedAd(json);

  std::move(callback).Run(ad_content.ToJson());
}

void BatAdsImpl::ToggleFlaggedAd(const std::string& json,
                                 ToggleFlaggedAdCallback callback) {
  ads::AdContentInfo ad_content;
  ad_content.FromJson(json);
  ad_content.is_flagged = ads_->ToggleFlaggedAd(json);

  std::move(callback).Run(ad_content.ToJson());
}

void BatAdsImpl::OnResourceComponentUpdated(const std::string& id) {
  ads_->OnResourceComponentUpdated(id);
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

void BatAdsImpl::OnGetInlineContentAd(
    CallbackHolder<GetInlineContentAdCallback>* holder,
    const bool success,
    const std::string& dimensions,
    const ads::InlineContentAdInfo& ad) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success, dimensions, ad.ToJson());
  }

  delete holder;
}

void BatAdsImpl::OnTriggerSearchResultAdEvent(
    CallbackHolder<TriggerSearchResultAdEventCallback>* holder,
    const bool success,
    const std::string& placement_id,
    const ads::mojom::SearchResultAdEventType event_type) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success, placement_id, event_type);
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
    const bool success,
    const ads::StatementInfo& statement) {
  if (holder->is_valid()) {
    const std::string json = statement.ToJson();
    std::move(holder->get()).Run(success, json);
  }

  delete holder;
}

// static
void BatAdsImpl::OnGetDiagnostics(
    CallbackHolder<GetDiagnosticsCallback>* holder,
    const bool success,
    const std::string& json) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(success, json);
  }

  delete holder;
}

}  // namespace bat_ads
