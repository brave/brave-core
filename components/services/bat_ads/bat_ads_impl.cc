/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <utility>
#include <vector>

#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"
#include "bat/ads/ad_content.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_history.h"
#include "bat/ads/category_content.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/mojom.h"

using std::placeholders::_1;

namespace bat_ads {

namespace {

ads::Result ToMojomResult(int32_t result) {
  return (ads::Result)result;
}

ads::AdContent::LikeAction ToAdsLikeAction(
    const int action) {
  return static_cast<ads::AdContent::LikeAction>(action);
}

ads::CategoryContent::OptAction ToAdsOptAction(
    const int action) {
  return static_cast<ads::CategoryContent::OptAction>(action);
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

void BatAdsImpl::SetConfirmationsIsReady(
    const bool is_ready) {
  ads_->SetConfirmationsIsReady(is_ready);
}

void BatAdsImpl::ChangeLocale(
    const std::string& locale) {
  ads_->ChangeLocale(locale);
}

void BatAdsImpl::OnAdsSubdivisionTargetingCodeHasChanged() {
  ads_->OnAdsSubdivisionTargetingCodeHasChanged();
}

void BatAdsImpl::OnPageLoaded(
    const std::string& url,
    const std::string& content) {
  ads_->OnPageLoaded(url, content);
}

void BatAdsImpl::OnUnIdle() {
  ads_->OnUnIdle();
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
    const bool is_incognito) {
  ads_->OnTabUpdated(tab_id, url, is_active, is_incognito);
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
    const ads::AdNotificationEventType event_type) {
  ads_->OnAdNotificationEvent(uuid, event_type);
}

void BatAdsImpl::RemoveAllHistory(
    RemoveAllHistoryCallback callback) {
  auto* holder = new CallbackHolder<RemoveAllHistoryCallback>(AsWeakPtr(),
      std::move(callback));

  auto remove_all_history_callback =
      std::bind(BatAdsImpl::OnRemoveAllHistory, holder, _1);
  ads_->RemoveAllHistory(remove_all_history_callback);
}

void BatAdsImpl::GetAdsHistory(
    const uint64_t from_timestamp,
    const uint64_t to_timestamp,
    GetAdsHistoryCallback callback) {
  ads::AdsHistory history = ads_->GetAdsHistory(
      ads::AdsHistory::FilterType::kConfirmationType,
          ads::AdsHistory::SortType::kDescendingOrder, from_timestamp,
              to_timestamp);

  std::move(callback).Run(history.ToJson());
}

void BatAdsImpl::ToggleAdThumbUp(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const int action,
    ToggleAdThumbUpCallback callback) {
  const ads::AdContent::LikeAction like_action = ads_->ToggleAdThumbUp(
      creative_instance_id, creative_set_id, ToAdsLikeAction(action));
  std::move(callback).Run(creative_instance_id, static_cast<int>(like_action));
}

void BatAdsImpl::ToggleAdThumbDown(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const int action,
    ToggleAdThumbDownCallback callback) {
  const ads::AdContent::LikeAction like_action = ads_->ToggleAdThumbDown(
      creative_instance_id, creative_set_id, ToAdsLikeAction(action));
  std::move(callback).Run(creative_instance_id, static_cast<int>(like_action));
}

void BatAdsImpl::ToggleAdOptInAction(
    const std::string& category,
    const int action,
    ToggleAdOptInActionCallback callback) {
  const ads::CategoryContent::OptAction opt_action =
      ads_->ToggleAdOptInAction(category, ToAdsOptAction(action));
  std::move(callback).Run(category, static_cast<int>(opt_action));
}

void BatAdsImpl::ToggleAdOptOutAction(
    const std::string& category,
    const int action,
    ToggleAdOptOutActionCallback callback) {
  const ads::CategoryContent::OptAction opt_action =
      ads_->ToggleAdOptOutAction(category, ToAdsOptAction(action));
  std::move(callback).Run(category, static_cast<int>(opt_action));
}

void BatAdsImpl::ToggleSaveAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool saved,
    ToggleSaveAdCallback callback) {
  const bool saved_result =
      ads_->ToggleSaveAd(creative_instance_id, creative_set_id, saved);
  std::move(callback).Run(creative_instance_id, saved_result);
}

void BatAdsImpl::ToggleFlagAd(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const bool flagged,
    ToggleFlagAdCallback callback) {
  bool flagged_result =
      ads_->ToggleFlagAd(creative_instance_id, creative_set_id, flagged);
  std::move(callback).Run(creative_instance_id, flagged_result);
}

void BatAdsImpl::OnUserModelUpdated(
    const std::string& id) {
  ads_->OnUserModelUpdated(id);
}

///////////////////////////////////////////////////////////////////////////////

void BatAdsImpl::OnInitialize(
    CallbackHolder<InitializeCallback>* holder,
    const int32_t result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

void BatAdsImpl::OnShutdown(
    CallbackHolder<ShutdownCallback>* holder,
    const int32_t result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

void BatAdsImpl::OnRemoveAllHistory(
    CallbackHolder<RemoveAllHistoryCallback>* holder,
    const int32_t result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

}  // namespace bat_ads
