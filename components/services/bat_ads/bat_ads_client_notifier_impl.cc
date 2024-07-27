/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_client_notifier_impl.h"

#include <utility>

namespace bat_ads {

BatAdsClientNotifierImpl::BatAdsClientNotifierImpl(
    mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier)
    : pending_receiver_(std::move(client_notifier)) {}

BatAdsClientNotifierImpl::~BatAdsClientNotifierImpl() = default;

void BatAdsClientNotifierImpl::AddObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  DCHECK(observer);
  notifier_.AddObserver(observer);
}

void BatAdsClientNotifierImpl::RemoveObserver(
    brave_ads::AdsClientNotifierObserver* observer) {
  DCHECK(observer);
  notifier_.RemoveObserver(observer);
}

void BatAdsClientNotifierImpl::BindReceiver() {
  DCHECK(pending_receiver_.is_valid());
  receiver_.Bind(std::move(pending_receiver_));
}

void BatAdsClientNotifierImpl::NotifyDidInitializeAds() {
  notifier_.NotifyDidInitializeAds();
}

void BatAdsClientNotifierImpl::NotifyLocaleDidChange(
    const std::string& locale) {
  notifier_.NotifyLocaleDidChange(locale);
}

void BatAdsClientNotifierImpl::NotifyPrefDidChange(const std::string& path) {
  notifier_.NotifyPrefDidChange(path);
}

void BatAdsClientNotifierImpl::NotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  notifier_.NotifyResourceComponentDidChange(manifest_version, id);
}

void BatAdsClientNotifierImpl::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  notifier_.NotifyDidUnregisterResourceComponent(id);
}

void BatAdsClientNotifierImpl::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  notifier_.NotifyRewardsWalletDidUpdate(payment_id, recovery_seed);
}

void BatAdsClientNotifierImpl::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  notifier_.NotifyTabTextContentDidChange(tab_id, redirect_chain, text);
}

void BatAdsClientNotifierImpl::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  notifier_.NotifyTabHtmlContentDidChange(tab_id, redirect_chain, html);
}

void BatAdsClientNotifierImpl::NotifyTabDidStartPlayingMedia(
    const int32_t tab_id) {
  notifier_.NotifyTabDidStartPlayingMedia(tab_id);
}

void BatAdsClientNotifierImpl::NotifyTabDidStopPlayingMedia(
    const int32_t tab_id) {
  notifier_.NotifyTabDidStopPlayingMedia(tab_id);
}

void BatAdsClientNotifierImpl::NotifyTabDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const bool is_new_navigation,
    const bool is_restoring,
    const bool is_error_page,
    const bool is_visible) {
  notifier_.NotifyTabDidChange(tab_id, redirect_chain, is_new_navigation,
                               is_restoring, is_error_page, is_visible);
}

void BatAdsClientNotifierImpl::NotifyDidCloseTab(const int32_t tab_id) {
  notifier_.NotifyDidCloseTab(tab_id);
}

void BatAdsClientNotifierImpl::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) {
  notifier_.NotifyUserGestureEventTriggered(page_transition_type);
}

void BatAdsClientNotifierImpl::NotifyUserDidBecomeIdle() {
  notifier_.NotifyUserDidBecomeIdle();
}

void BatAdsClientNotifierImpl::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) {
  notifier_.NotifyUserDidBecomeActive(idle_time, screen_was_locked);
}

void BatAdsClientNotifierImpl::NotifyBrowserDidEnterForeground() {
  notifier_.NotifyBrowserDidEnterForeground();
}

void BatAdsClientNotifierImpl::NotifyBrowserDidEnterBackground() {
  notifier_.NotifyBrowserDidEnterBackground();
}

void BatAdsClientNotifierImpl::NotifyBrowserDidBecomeActive() {
  notifier_.NotifyBrowserDidBecomeActive();
}

void BatAdsClientNotifierImpl::NotifyBrowserDidResignActive() {
  notifier_.NotifyBrowserDidResignActive();
}

void BatAdsClientNotifierImpl::NotifyDidSolveAdaptiveCaptcha() {
  notifier_.NotifyDidSolveAdaptiveCaptcha();
}

}  // namespace bat_ads
