/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_WAITER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_WAITER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

class GURL;

namespace brave_ads {

class AdsClientNotifierInterface;

namespace test {

class AdsClientNotifierWaiter final : public AdsClientNotifierObserver {
 public:
  explicit AdsClientNotifierWaiter(
      AdsClientNotifierInterface* ads_client_notifier);

  AdsClientNotifierWaiter(const AdsClientNotifierWaiter&) = delete;
  AdsClientNotifierWaiter& operator=(const AdsClientNotifierWaiter&) = delete;

  ~AdsClientNotifierWaiter() override;

  void WaitForOnNotifyDidInitializeAds();
  void WaitForOnNotifyLocaleDidChange();
  void WaitForOnNotifyPrefDidChange();
  void WaitForOnNotifyResourceComponentDidChange();
  void WaitForOnNotifyDidUnregisterResourceComponent();
  void WaitForOnNotifyRewardsWalletDidUpdate();
  void WaitForOnNotifyTabTextContentDidChange();
  void WaitForOnNotifyTabHtmlContentDidChange();
  void WaitForOnNotifyTabDidStartPlayingMedia();
  void WaitForOnNotifyTabDidStopPlayingMedia();
  void WaitForOnNotifyTabDidChange();
  void WaitForOnNotifyTabDidLoad();
  void WaitForOnNotifyDidCloseTab();
  void WaitForOnNotifyUserGestureEventTriggered();
  void WaitForOnNotifyUserDidBecomeIdle();
  void WaitForOnNotifyUserDidBecomeActive();
  void WaitForOnNotifyBrowserDidEnterForeground();
  void WaitForOnNotifyBrowserDidEnterBackground();
  void WaitForOnNotifyBrowserDidBecomeActive();
  void WaitForOnNotifyBrowserDidResignActive();
  void WaitForOnNotifyDidSolveAdaptiveCaptcha();

 private:
  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyLocaleDidChange(const std::string& locale) override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyResourceComponentDidChange(const std::string& manifest_version,
                                          const std::string& id) override;
  void OnNotifyDidUnregisterResourceComponent(const std::string& id) override;
  void OnNotifyRewardsWalletDidUpdate(
      const std::string& payment_id,
      const std::string& recovery_seed_base64) override;
  void OnNotifyTabTextContentDidChange(int32_t tab_id,
                                       const std::vector<GURL>& redirect_chain,
                                       const std::string& text) override;
  void OnNotifyTabHtmlContentDidChange(int32_t tab_id,
                                       const std::vector<GURL>& redirect_chain,
                                       const std::string& html) override;
  void OnNotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void OnNotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void OnNotifyTabDidChange(int32_t tab_id,
                            const std::vector<GURL>& redirect_chain,
                            bool is_new_navigation,
                            bool is_restoring,
                            bool is_visible) override;
  void OnNotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void OnNotifyDidCloseTab(int32_t tab_id) override;
  void OnNotifyUserGestureEventTriggered(int32_t page_transition_type) override;
  void OnNotifyUserDidBecomeIdle() override;
  void OnNotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                   bool screen_was_locked) override;
  void OnNotifyBrowserDidEnterForeground() override;
  void OnNotifyBrowserDidEnterBackground() override;
  void OnNotifyBrowserDidBecomeActive() override;
  void OnNotifyBrowserDidResignActive() override;
  void OnNotifyDidSolveAdaptiveCaptcha() override;

  base::RunLoop on_notify_did_initialize_ads_run_loop_;
  base::RunLoop on_notify_locale_did_change_run_loop_;
  base::RunLoop on_notify_pref_did_change_run_loop_;
  base::RunLoop on_notify_resource_component_did_change_run_loop_;
  base::RunLoop on_notify_did_unregister_resource_component_run_loop_;
  base::RunLoop on_notify_rewards_wallet_did_update_run_loop_;
  base::RunLoop on_notify_tab_text_content_did_change_run_loop_;
  base::RunLoop on_notify_tab_html_content_did_change_run_loop_;
  base::RunLoop on_notify_tab_did_start_playing_media_run_loop_;
  base::RunLoop on_notify_tab_did_stop_playing_media_run_loop_;
  base::RunLoop on_notify_tab_did_change_run_loop_;
  base::RunLoop on_notify_tab_did_load_run_loop_;
  base::RunLoop on_notify_did_close_tab_run_loop_;
  base::RunLoop on_notify_user_gesture_event_triggered_run_loop_;
  base::RunLoop on_notify_user_did_become_idle_run_loop_;
  base::RunLoop on_notify_user_did_become_active_run_loop_;
  base::RunLoop on_notify_browser_did_enter_foreground_run_loop_;
  base::RunLoop on_notify_browser_did_enter_background_run_loop_;
  base::RunLoop on_notify_browser_did_become_active_run_loop_;
  base::RunLoop on_notify_browser_did_resign_active_run_loop_;
  base::RunLoop on_notify_did_solve_adaptive_captcha_run_loop_;

  base::ScopedObservation<AdsClientNotifierInterface, AdsClientNotifierObserver>
      observation_{this};
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_WAITER_H_
