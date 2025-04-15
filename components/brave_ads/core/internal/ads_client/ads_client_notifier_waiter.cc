/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_waiter.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_for_testing.h"

namespace brave_ads::test {

AdsClientNotifierWaiter::AdsClientNotifierWaiter(
    AdsClientNotifierInterface* const ads_client_notifier) {
  CHECK(ads_client_notifier);

  observation_.Observe(ads_client_notifier);
}

AdsClientNotifierWaiter::~AdsClientNotifierWaiter() = default;

void AdsClientNotifierWaiter::WaitForOnNotifyDidInitializeAds() {
  on_notify_did_initialize_ads_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyLocaleDidChange() {
  on_notify_locale_did_change_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyPrefDidChange() {
  on_notify_pref_did_change_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyResourceComponentDidChange() {
  on_notify_resource_component_did_change_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyDidUnregisterResourceComponent() {
  on_notify_did_unregister_resource_component_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyRewardsWalletDidUpdate() {
  on_notify_rewards_wallet_did_update_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyTabTextContentDidChange() {
  on_notify_tab_text_content_did_change_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyTabHtmlContentDidChange() {
  on_notify_tab_html_content_did_change_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyTabDidStartPlayingMedia() {
  on_notify_tab_did_start_playing_media_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyTabDidStopPlayingMedia() {
  on_notify_tab_did_stop_playing_media_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyTabDidChange() {
  on_notify_tab_did_change_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyTabDidLoad() {
  on_notify_tab_did_load_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyDidCloseTab() {
  on_notify_did_close_tab_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyUserGestureEventTriggered() {
  on_notify_user_gesture_event_triggered_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyUserDidBecomeIdle() {
  on_notify_user_did_become_idle_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyUserDidBecomeActive() {
  on_notify_user_did_become_active_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyBrowserDidEnterForeground() {
  on_notify_browser_did_enter_foreground_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyBrowserDidEnterBackground() {
  on_notify_browser_did_enter_background_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyBrowserDidBecomeActive() {
  on_notify_browser_did_become_active_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyBrowserDidResignActive() {
  on_notify_browser_did_resign_active_run_loop_.Run();
}

void AdsClientNotifierWaiter::WaitForOnNotifyDidSolveAdaptiveCaptcha() {
  on_notify_did_solve_adaptive_captcha_run_loop_.Run();
}

///////////////////////////////////////////////////////////////////////////////

void AdsClientNotifierWaiter::OnNotifyDidInitializeAds() {
  on_notify_did_initialize_ads_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyLocaleDidChange(
    const std::string& /*locale*/) {
  on_notify_pref_did_change_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyPrefDidChange(
    const std::string& /*path*/) {
  on_notify_pref_did_change_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyResourceComponentDidChange(
    const std::string& /*manifest_version*/,
    const std::string& /*id*/) {
  on_notify_resource_component_did_change_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyDidUnregisterResourceComponent(
    const std::string& /*id*/) {
  on_notify_did_unregister_resource_component_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyRewardsWalletDidUpdate(
    const std::string& /*payment_id*/,
    const std::string& /*recovery_seed_base64*/) {
  on_notify_rewards_wallet_did_update_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyTabTextContentDidChange(
    int32_t /*tab_id*/,
    const std::vector<GURL>& /*redirect_chain*/,
    const std::string& /*text*/) {
  on_notify_tab_text_content_did_change_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyTabHtmlContentDidChange(
    int32_t /*tab_id*/,
    const std::vector<GURL>& /*redirect_chain*/,
    const std::string& /*html*/) {
  on_notify_tab_html_content_did_change_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyTabDidStartPlayingMedia(
    int32_t /*tab_id*/) {
  on_notify_tab_did_start_playing_media_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyTabDidStopPlayingMedia(
    int32_t /*tab_id*/) {
  on_notify_tab_did_stop_playing_media_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyTabDidChange(
    int32_t /*tab_id*/,
    const std::vector<GURL>& /*redirect_chain*/,
    bool /*is_new_navigation*/,
    bool /*is_restoring*/,
    bool /*is_visible*/) {
  on_notify_tab_did_change_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyTabDidLoad(int32_t /*tab_id*/,
                                                 int /*http_status_code*/) {
  on_notify_tab_did_load_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyDidCloseTab(int32_t /*tab_id*/) {
  on_notify_did_close_tab_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyUserGestureEventTriggered(
    int32_t /*page_transition_type*/) {
  on_notify_user_gesture_event_triggered_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyUserDidBecomeIdle() {
  on_notify_user_did_become_idle_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyUserDidBecomeActive(
    base::TimeDelta /*idle_time*/,
    bool /*screen_was_locked*/) {
  on_notify_user_did_become_active_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyBrowserDidEnterForeground() {
  on_notify_browser_did_enter_foreground_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyBrowserDidEnterBackground() {
  on_notify_browser_did_enter_background_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyBrowserDidBecomeActive() {
  on_notify_browser_did_become_active_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyBrowserDidResignActive() {
  on_notify_browser_did_resign_active_run_loop_.Quit();
}

void AdsClientNotifierWaiter::OnNotifyDidSolveAdaptiveCaptcha() {
  on_notify_did_solve_adaptive_captcha_run_loop_.Quit();
}

}  // namespace brave_ads::test
