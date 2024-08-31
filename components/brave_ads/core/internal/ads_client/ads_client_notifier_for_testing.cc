/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_for_testing.h"

#include "url/gurl.h"

namespace brave_ads {

void AdsClientNotifierForTesting::NotifyDidInitializeAds() {
  AdsClientNotifier::NotifyDidInitializeAds();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed_base64) {
  AdsClientNotifier::NotifyRewardsWalletDidUpdate(payment_id,
                                                  recovery_seed_base64);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyLocaleDidChange(
    const std::string& locale) {
  AdsClientNotifier::NotifyLocaleDidChange(locale);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyPrefDidChange(const std::string& path) {
  AdsClientNotifier::NotifyPrefDidChange(path);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  AdsClientNotifier::NotifyResourceComponentDidChange(manifest_version, id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  AdsClientNotifier::NotifyDidUnregisterResourceComponent(id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  AdsClientNotifier::NotifyTabTextContentDidChange(tab_id, redirect_chain,
                                                   text);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  AdsClientNotifier::NotifyTabHtmlContentDidChange(tab_id, redirect_chain,
                                                   html);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabDidStartPlayingMedia(
    const int32_t tab_id) {
  AdsClientNotifier::NotifyTabDidStartPlayingMedia(tab_id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabDidStopPlayingMedia(
    const int32_t tab_id) {
  AdsClientNotifier::NotifyTabDidStopPlayingMedia(tab_id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const bool is_new_navigation,
    const bool is_restoring,
    const bool is_error_page,
    const bool is_visible) {
  AdsClientNotifier::NotifyTabDidChange(tab_id, redirect_chain,
                                        is_new_navigation, is_restoring,
                                        is_error_page, is_visible);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyDidCloseTab(const int32_t tab_id) {
  AdsClientNotifier::NotifyDidCloseTab(tab_id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) {
  AdsClientNotifier::NotifyUserGestureEventTriggered(page_transition_type);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyUserDidBecomeIdle() {
  AdsClientNotifier::NotifyUserDidBecomeIdle();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) {
  AdsClientNotifier::NotifyUserDidBecomeActive(idle_time, screen_was_locked);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidEnterForeground() {
  AdsClientNotifier::NotifyBrowserDidEnterForeground();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidEnterBackground() {
  AdsClientNotifier::NotifyBrowserDidEnterBackground();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidBecomeActive() {
  AdsClientNotifier::NotifyBrowserDidBecomeActive();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidResignActive() {
  AdsClientNotifier::NotifyBrowserDidResignActive();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyDidSolveAdaptiveCaptcha() {
  AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha();

  RunTaskEnvironmentUntilIdle();
}

///////////////////////////////////////////////////////////////////////////////

void AdsClientNotifierForTesting::RunTaskEnvironmentUntilIdle() {
  CHECK(task_environment_)
      << "set_ads_client_notifier_task_environment must be set before calling "
         "AdsClientNotifierForTesting::Notify* functions";
  task_environment_->RunUntilIdle();
}

}  // namespace brave_ads
