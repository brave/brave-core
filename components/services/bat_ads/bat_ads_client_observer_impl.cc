/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/services/bat_ads/bat_ads_client_observer_impl.h"

namespace bat_ads {

BatAdsClientObserverImpl::BatAdsClientObserverImpl() = default;

BatAdsClientObserverImpl::~BatAdsClientObserverImpl() = default;

mojo::PendingRemote<bat_ads::mojom::BatAdsClientObserver>
BatAdsClientObserverImpl::CreatePendingReceiverAndPassRemote() {
  return pending_receiver_.InitWithNewPipeAndPassRemote();
}

void BatAdsClientObserverImpl::BindReceiver() {
  DCHECK(pending_receiver_.is_valid());
  receiver_.Bind(std::move(pending_receiver_));
}

void BatAdsClientObserverImpl::AddObserver(ads::AdsClientObserver* observer) {
  DCHECK(observer);
  observer_manager_.AddObserver(observer);
}

void BatAdsClientObserverImpl::RemoveObserver(
    ads::AdsClientObserver* observer) {
  DCHECK(observer);
  observer_manager_.RemoveObserver(observer);
}

void BatAdsClientObserverImpl::NotifyLocaleDidChange(
    const std::string& locale) {
  observer_manager_.NotifyLocaleDidChange(locale);
}

void BatAdsClientObserverImpl::NotifyPrefDidChange(const std::string& path) {
  observer_manager_.NotifyPrefDidChange(path);
}

void BatAdsClientObserverImpl::NotifyDidUpdateResourceComponent(
    const std::string& id) {
  observer_manager_.NotifyDidUpdateResourceComponent(id);
}

void BatAdsClientObserverImpl::NotifyTabTextContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  observer_manager_.NotifyTabTextContentDidChange(tab_id, redirect_chain, text);
}

void BatAdsClientObserverImpl::NotifyTabHtmlContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  observer_manager_.NotifyTabTextContentDidChange(tab_id, redirect_chain, html);
}

void BatAdsClientObserverImpl::NotifyTabDidStartPlayingMedia(int32_t tab_id) {
  observer_manager_.NotifyTabDidStartPlayingMedia(tab_id);
}

void BatAdsClientObserverImpl::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  observer_manager_.NotifyTabDidStopPlayingMedia(tab_id);
}

void BatAdsClientObserverImpl::NotifyTabDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    bool is_visible,
    bool is_incognito) {
  observer_manager_.NotifyTabDidChange(tab_id, redirect_chain, is_visible,
                                       is_incognito);
}

void BatAdsClientObserverImpl::NotifyDidCloseTab(int32_t tab_id) {
  observer_manager_.NotifyDidCloseTab(tab_id);
}

void BatAdsClientObserverImpl::NotifyUserDidBecomeIdle() {
  observer_manager_.NotifyUserDidBecomeIdle();
}

void BatAdsClientObserverImpl::NotifyUserDidBecomeActive(
    base::TimeDelta idle_time,
    bool screen_was_locked) {
  observer_manager_.NotifyUserDidBecomeActive(idle_time, screen_was_locked);
}

void BatAdsClientObserverImpl::NotifyBrowserDidEnterForeground() {
  observer_manager_.NotifyBrowserDidEnterForeground();
}

void BatAdsClientObserverImpl::NotifyBrowserDidEnterBackground() {
  observer_manager_.NotifyBrowserDidEnterBackground();
}

void BatAdsClientObserverImpl::NotifyBrowserDidBecomeActive() {
  observer_manager_.NotifyBrowserDidBecomeActive();
}

void BatAdsClientObserverImpl::NotifyBrowserDidResignActive() {
  observer_manager_.NotifyBrowserDidResignActive();
}

void BatAdsClientObserverImpl::NotifyRewardsWalletIsReady(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  observer_manager_.NotifyRewardsWalletIsReady(payment_id, recovery_seed);
}

void BatAdsClientObserverImpl::NotifyRewardsWalletDidChange(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  observer_manager_.NotifyRewardsWalletDidChange(payment_id, recovery_seed);
}

}  // namespace bat_ads
