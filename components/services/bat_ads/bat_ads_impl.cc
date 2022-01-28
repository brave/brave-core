/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <utility>
#include <vector>

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_history_filter_types.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/ads_history_sort_types.h"
#include "bat/ads/category_content_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/new_tab_page_ad_info.h"
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
                              const std::vector<std::string>& redirect_chain,
                              const std::string& html) {
  ads_->OnHtmlLoaded(tab_id, redirect_chain, html);
}

void BatAdsImpl::OnTextLoaded(const int32_t tab_id,
                              const std::vector<std::string>& redirect_chain,
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

void BatAdsImpl::OnForeground() {
  ads_->OnForeground();
}

void BatAdsImpl::OnBackground() {
  ads_->OnBackground();
}

void BatAdsImpl::OnMediaPlaying(
    const int32_t tab_id) {
  ads_->OnMediaPlaying(tab_id);
}

void BatAdsImpl::OnMediaStopped(
    const int32_t tab_id) {
  ads_->OnMediaStopped(tab_id);
}

void BatAdsImpl::OnTabUpdated(
    const int32_t tab_id,
    const std::string& url,
    const bool is_active,
    const bool is_browser_active,
    const bool is_incognito) {
  ads_->OnTabUpdated(tab_id, url, is_active, is_browser_active, is_incognito);
}

void BatAdsImpl::OnTabClosed(
    const int32_t tab_id) {
  ads_->OnTabClosed(tab_id);
}

void BatAdsImpl::GetAdNotification(
    const std::string& uuid,
    GetAdNotificationCallback callback) {
  ads::AdNotificationInfo notification;
  ads_->GetAdNotification(uuid, &notification);
  std::move(callback).Run(notification.ToJson());
}

void BatAdsImpl::OnAdNotificationEvent(
    const std::string& uuid,
    const ads::mojom::AdNotificationEventType event_type) {
  ads_->OnAdNotificationEvent(uuid, event_type);
}

void BatAdsImpl::GetNewTabPageAd(GetNewTabPageAdCallback callback) {
  auto* holder = new CallbackHolder<GetNewTabPageAdCallback>(
      AsWeakPtr(), std::move(callback));

  auto get_new_tab_page_ad_callback =
      std::bind(BatAdsImpl::OnGetNewTabPageAd, holder, _1, _2);
  ads_->GetNewTabPageAd(get_new_tab_page_ad_callback);
}

void BatAdsImpl::OnNewTabPageAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const ads::mojom::NewTabPageAdEventType event_type) {
  ads_->OnNewTabPageAdEvent(uuid, creative_instance_id, event_type);
}

void BatAdsImpl::OnPromotedContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const ads::mojom::PromotedContentAdEventType event_type) {
  ads_->OnPromotedContentAdEvent(uuid, creative_instance_id, event_type);
}

void BatAdsImpl::GetInlineContentAd(const std::string& dimensions,
                                    GetInlineContentAdCallback callback) {
  auto* holder = new CallbackHolder<GetInlineContentAdCallback>(
      AsWeakPtr(), std::move(callback));

  auto get_inline_content_ads_callback =
      std::bind(BatAdsImpl::OnGetInlineContentAd, holder, _1, _2, _3);
  ads_->GetInlineContentAd(dimensions, get_inline_content_ads_callback);
}

void BatAdsImpl::OnInlineContentAdEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const ads::mojom::InlineContentAdEventType event_type) {
  ads_->OnInlineContentAdEvent(uuid, creative_instance_id, event_type);
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

void BatAdsImpl::GetAdsHistory(const double from_timestamp,
                               const double to_timestamp,
                               GetAdsHistoryCallback callback) {
  ads::AdsHistoryInfo history = ads_->GetAdsHistory(
      ads::AdsHistoryFilterType::kConfirmationType,
      ads::AdsHistorySortType::kDescendingOrder, from_timestamp, to_timestamp);

  std::move(callback).Run(history.ToJson());
}

void BatAdsImpl::GetAccountStatement(GetAccountStatementCallback callback) {
  auto* holder = new CallbackHolder<GetAccountStatementCallback>(
      AsWeakPtr(), std::move(callback));

  ads_->GetAccountStatement(
      std::bind(BatAdsImpl::OnGetAccountStatement, holder, _1, _2));
}

void BatAdsImpl::GetAdDiagnostics(GetAdDiagnosticsCallback callback) {
  auto* holder = new CallbackHolder<GetAdDiagnosticsCallback>(
      AsWeakPtr(), std::move(callback));

  ads_->GetAdDiagnostics(
      std::bind(BatAdsImpl::OnGetAdDiagnostics, holder, _1, _2));
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

// static
void BatAdsImpl::OnGetNewTabPageAd(
    CallbackHolder<GetNewTabPageAdCallback>* holder,
    const bool success,
    const ads::NewTabPageAdInfo& ad) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success, ad.ToJson());
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

void BatAdsImpl::OnRemoveAllHistory(
    CallbackHolder<RemoveAllHistoryCallback>* holder,
    const bool success) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(success);
  }

  delete holder;
}

void BatAdsImpl::OnGetAccountStatement(
    CallbackHolder<GetAccountStatementCallback>* holder,
    const bool success,
    const ads::StatementInfo& statement) {
  if (holder->is_valid()) {
    const std::string json = statement.ToJson();
    std::move(holder->get()).Run(success, json);
  }

  delete holder;
}

// static
void BatAdsImpl::OnGetAdDiagnostics(
    CallbackHolder<GetAdDiagnosticsCallback>* holder,
    const bool success,
    const std::string& json) {
  DCHECK(holder);

  if (holder->is_valid()) {
    std::move(holder->get()).Run(success, json);
  }

  delete holder;
}

}  // namespace bat_ads
