/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_impl.h"

#include <utility>

#include "bat/ads/ads.h"
#include "brave/components/services/bat_ads/bat_ads_client_mojo_bridge.h"

using std::placeholders::_1;

namespace bat_ads {

namespace {

ads::Result ToMojomResult(int32_t result) {
  return (ads::Result)result;
}

ads::NotificationEventType ToMojomNotificationEventType(int32_t event_type) {
  return (ads::NotificationEventType)event_type;
}

}  // namespace

BatAdsImpl::BatAdsImpl(mojom::BatAdsClientAssociatedPtrInfo client_info) :
    bat_ads_client_mojo_proxy_(new BatAdsClientMojoBridge(
        std::move(client_info))),
    ads_(ads::Ads::CreateInstance(bat_ads_client_mojo_proxy_.get())) {}

BatAdsImpl::~BatAdsImpl() {}

void BatAdsImpl::Initialize(InitializeCallback callback) {
  auto* holder = new CallbackHolder<InitializeCallback>(AsWeakPtr(),
      std::move(callback));

  auto initialize_callback = std::bind(BatAdsImpl::OnInitialize, holder, _1);
  ads_->Initialize(initialize_callback);
}

void BatAdsImpl::OnInitialize(
    CallbackHolder<InitializeCallback>* holder,
    const int32_t result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

void BatAdsImpl::Shutdown(ShutdownCallback callback) {
  auto* holder = new CallbackHolder<ShutdownCallback>(AsWeakPtr(),
      std::move(callback));

  auto shutdown_callback = std::bind(BatAdsImpl::OnShutdown, holder, _1);
  ads_->Shutdown(shutdown_callback);
}

void BatAdsImpl::OnShutdown(
    CallbackHolder<ShutdownCallback>* holder,
    const int32_t result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(ToMojomResult(result));
  }

  delete holder;
}

void BatAdsImpl::SetConfirmationsIsReady(const bool is_ready) {
  ads_->SetConfirmationsIsReady(is_ready);
}

void BatAdsImpl::ChangeLocale(const std::string& locale) {
  ads_->ChangeLocale(locale);
}

void BatAdsImpl::ClassifyPage(
    const std::string& url,
    const std::string& page) {
  ads_->ClassifyPage(url, page);
}

void BatAdsImpl::ServeSampleAd() {
  ads_->ServeSampleAd();
}

void BatAdsImpl::OnTimer(const uint32_t timer_id) {
  ads_->OnTimer(timer_id);
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

void BatAdsImpl::OnMediaPlaying(const int32_t tab_id) {
  ads_->OnMediaPlaying(tab_id);
}

void BatAdsImpl::OnMediaStopped(const int32_t tab_id) {
  ads_->OnMediaStopped(tab_id);
}

void BatAdsImpl::OnTabUpdated(
    const int32_t tab_id,
    const std::string& url,
    const bool is_active,
    const bool is_incognito) {
  ads_->OnTabUpdated(tab_id, url, is_active, is_incognito);
}

void BatAdsImpl::OnTabClosed(const int32_t tab_id) {
  ads_->OnTabClosed(tab_id);
}

void BatAdsImpl::GetNotificationForId(
    const std::string& id,
    GetNotificationForIdCallback callback) {
  ads::NotificationInfo notification;
  ads_->GetNotificationForId(id, &notification);
  std::move(callback).Run(notification.ToJson());
}

void BatAdsImpl::OnNotificationEvent(
    const std::string& id,
    const int32_t type) {
  ads_->OnNotificationEvent(id, ToMojomNotificationEventType(type));
}

void BatAdsImpl::RemoveAllHistory(RemoveAllHistoryCallback callback) {
  auto* holder = new CallbackHolder<RemoveAllHistoryCallback>(AsWeakPtr(),
      std::move(callback));

  auto remove_all_history_callback =
      std::bind(BatAdsImpl::OnRemoveAllHistory, holder, _1);
  ads_->RemoveAllHistory(remove_all_history_callback);
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
