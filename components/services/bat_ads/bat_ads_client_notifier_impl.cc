/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_notifier_impl.h"

#include <utility>

namespace bat_ads {

BatAdsClientNotifierImpl::BatAdsClientNotifierImpl(
    mojo::PendingReceiver<mojom::BatAdsClientNotifier>
        bat_ads_client_notifier_pending_receiver)
    : bat_ads_client_notifier_pending_receiver_(
          std::move(bat_ads_client_notifier_pending_receiver)) {
  bat_ads_client_notifier_receiver_.Bind(
      std::move(bat_ads_client_notifier_pending_receiver_));
}

BatAdsClientNotifierImpl::~BatAdsClientNotifierImpl() = default;

void BatAdsClientNotifierImpl::AddObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  CHECK(observer);

  ads_client_notifier_.AddObserver(observer);
}

void BatAdsClientNotifierImpl::RemoveObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  CHECK(observer);

  ads_client_notifier_.RemoveObserver(observer);
}

void BatAdsClientNotifierImpl::NotifyPendingObservers() {
  ads_client_notifier_.NotifyPendingObservers();
}

void BatAdsClientNotifierImpl::NotifyDidInitializeAds() {
  ads_client_notifier_.NotifyDidInitializeAds();
}

void BatAdsClientNotifierImpl::NotifyLocaleDidChange(
    const std::string& locale) {
  ads_client_notifier_.NotifyLocaleDidChange(locale);
}

void BatAdsClientNotifierImpl::NotifyPrefDidChange(const std::string& path) {
  ads_client_notifier_.NotifyPrefDidChange(path);
}

void BatAdsClientNotifierImpl::NotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  ads_client_notifier_.NotifyResourceComponentDidChange(manifest_version, id);
}

void BatAdsClientNotifierImpl::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(id);
}

void BatAdsClientNotifierImpl::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed_base64) {
  ads_client_notifier_.NotifyRewardsWalletDidUpdate(payment_id,
                                                    recovery_seed_base64);
}

void BatAdsClientNotifierImpl::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  ads_client_notifier_.NotifyTabTextContentDidChange(tab_id, redirect_chain,
                                                     text);
}

void BatAdsClientNotifierImpl::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  ads_client_notifier_.NotifyTabHtmlContentDidChange(tab_id, redirect_chain,
                                                     html);
}

void BatAdsClientNotifierImpl::NotifyTabDidStartPlayingMedia(
    const int32_t tab_id) {
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(tab_id);
}

void BatAdsClientNotifierImpl::NotifyTabDidStopPlayingMedia(
    const int32_t tab_id) {
  ads_client_notifier_.NotifyTabDidStopPlayingMedia(tab_id);
}

void BatAdsClientNotifierImpl::NotifyTabDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const bool is_new_navigation,
    const bool is_restoring,
    const bool is_visible) {
  ads_client_notifier_.NotifyTabDidChange(
      tab_id, redirect_chain, is_new_navigation, is_restoring, is_visible);
}

void BatAdsClientNotifierImpl::NotifyTabDidLoad(const int32_t tab_id,
                                                const int http_status_code) {
  ads_client_notifier_.NotifyTabDidLoad(tab_id, http_status_code);
}

void BatAdsClientNotifierImpl::NotifyDidCloseTab(const int32_t tab_id) {
  ads_client_notifier_.NotifyDidCloseTab(tab_id);
}

void BatAdsClientNotifierImpl::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) {
  ads_client_notifier_.NotifyUserGestureEventTriggered(page_transition_type);
}

void BatAdsClientNotifierImpl::NotifyUserDidBecomeIdle() {
  ads_client_notifier_.NotifyUserDidBecomeIdle();
}

void BatAdsClientNotifierImpl::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) {
  ads_client_notifier_.NotifyUserDidBecomeActive(idle_time, screen_was_locked);
}

void BatAdsClientNotifierImpl::NotifyBrowserDidEnterForeground() {
  ads_client_notifier_.NotifyBrowserDidEnterForeground();
}

void BatAdsClientNotifierImpl::NotifyBrowserDidEnterBackground() {
  ads_client_notifier_.NotifyBrowserDidEnterBackground();
}

void BatAdsClientNotifierImpl::NotifyBrowserDidBecomeActive() {
  ads_client_notifier_.NotifyBrowserDidBecomeActive();
}

void BatAdsClientNotifierImpl::NotifyBrowserDidResignActive() {
  ads_client_notifier_.NotifyBrowserDidResignActive();
}

void BatAdsClientNotifierImpl::NotifyDidSolveAdaptiveCaptcha() {
  ads_client_notifier_.NotifyDidSolveAdaptiveCaptcha();
}

}  // namespace bat_ads
