/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/unittest/ads_client_observer_notifier_for_testing.h"

namespace ads {

namespace {}  // namespace

void AdsClientObserverNotifierForTesting::NotifyLocaleDidChange(
    const std::string& locale) {
  AdsClientObserverNotifier::NotifyLocaleDidChange(locale);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyPrefDidChange(
    const std::string& path) {
  AdsClientObserverNotifier::NotifyPrefDidChange(path);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyDidUpdateResourceComponent(
    const std::string& id) {
  AdsClientObserverNotifier::NotifyDidUpdateResourceComponent(id);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  AdsClientObserverNotifier::NotifyTabTextContentDidChange(
      tab_id, redirect_chain, text);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  AdsClientObserverNotifier::NotifyTabHtmlContentDidChange(
      tab_id, redirect_chain, html);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyTabDidStartPlayingMedia(
    const int32_t tab_id) {
  AdsClientObserverNotifier::NotifyTabDidStartPlayingMedia(tab_id);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyTabDidStopPlayingMedia(
    const int32_t tab_id) {
  AdsClientObserverNotifier::NotifyTabDidStopPlayingMedia(tab_id);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyTabDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const bool is_visible,
    const bool is_incognito) {
  AdsClientObserverNotifier::NotifyTabDidChange(tab_id, redirect_chain,
                                                is_visible, is_incognito);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyDidCloseTab(
    const int32_t tab_id) {
  AdsClientObserverNotifier::NotifyDidCloseTab(tab_id);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyUserDidBecomeIdle() {
  AdsClientObserverNotifier::NotifyUserDidBecomeIdle();
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) {
  AdsClientObserverNotifier::NotifyUserDidBecomeActive(idle_time,
                                                       screen_was_locked);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyBrowserDidEnterForeground() {
  AdsClientObserverNotifier::NotifyBrowserDidEnterForeground();
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyBrowserDidEnterBackground() {
  AdsClientObserverNotifier::NotifyBrowserDidEnterBackground();
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyBrowserDidBecomeActive() {
  AdsClientObserverNotifier::NotifyBrowserDidBecomeActive();
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyBrowserDidResignActive() {
  AdsClientObserverNotifier::NotifyBrowserDidResignActive();
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyRewardsWalletIsReady(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  AdsClientObserverNotifier::NotifyRewardsWalletIsReady(payment_id,
                                                        recovery_seed);
  FlushObserversForTesting();  // IN-TEST
}

void AdsClientObserverNotifierForTesting::NotifyRewardsWalletDidChange(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  AdsClientObserverNotifier::NotifyRewardsWalletDidChange(payment_id,
                                                          recovery_seed);
  FlushObserversForTesting();  // IN-TEST
}

///////////////////////////////////////////////////////////////////////////////

void AdsClientObserverNotifierForTesting::FlushObserversForTesting() {
  observers_.FlushForTesting();  // IN-TEST
}

}  // namespace ads
