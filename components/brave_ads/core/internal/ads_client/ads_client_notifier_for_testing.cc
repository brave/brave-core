/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_for_testing.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"
#include "url/gurl.h"

namespace brave_ads {

AdsClientNotifierForTesting::AdsClientNotifierForTesting() = default;

AdsClientNotifierForTesting::~AdsClientNotifierForTesting() = default;

void AdsClientNotifierForTesting::AddObserver(
    AdsClientNotifierObserver* const observer) {
  ads_client_notifier_.AddObserver(observer);
}

void AdsClientNotifierForTesting::RemoveObserver(
    AdsClientNotifierObserver* const observer) {
  ads_client_notifier_.RemoveObserver(observer);
}

void AdsClientNotifierForTesting::NotifyPendingObservers() {
  ads_client_notifier_.NotifyPendingObservers();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyDidInitializeAds() {
  ads_client_notifier_.NotifyDidInitializeAds();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed_base64) {
  ads_client_notifier_.NotifyRewardsWalletDidUpdate(payment_id,
                                                    recovery_seed_base64);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyLocaleDidChange(
    const std::string& locale) {
  ads_client_notifier_.NotifyLocaleDidChange(locale);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyPrefDidChange(const std::string& path) {
  ads_client_notifier_.NotifyPrefDidChange(path);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  ads_client_notifier_.NotifyResourceComponentDidChange(manifest_version, id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabTextContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  ads_client_notifier_.NotifyTabTextContentDidChange(tab_id, redirect_chain,
                                                     text);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabHtmlContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  ads_client_notifier_.NotifyTabHtmlContentDidChange(tab_id, redirect_chain,
                                                     html);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabDidStartPlayingMedia(
    int32_t tab_id) {
  ads_client_notifier_.NotifyTabDidStartPlayingMedia(tab_id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  ads_client_notifier_.NotifyTabDidStopPlayingMedia(tab_id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    bool is_new_navigation,
    bool is_restoring,
    bool is_visible) {
  ads_client_notifier_.NotifyTabDidChange(
      tab_id, redirect_chain, is_new_navigation, is_restoring, is_visible);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyTabDidLoad(int32_t tab_id,
                                                   int http_status_code) {
  ads_client_notifier_.NotifyTabDidLoad(tab_id, http_status_code);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyDidCloseTab(int32_t tab_id) {
  ads_client_notifier_.NotifyDidCloseTab(tab_id);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyUserGestureEventTriggered(
    int32_t page_transition_type) {
  ads_client_notifier_.NotifyUserGestureEventTriggered(page_transition_type);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyUserDidBecomeIdle() {
  ads_client_notifier_.NotifyUserDidBecomeIdle();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyUserDidBecomeActive(
    base::TimeDelta idle_time,
    bool screen_was_locked) {
  ads_client_notifier_.NotifyUserDidBecomeActive(idle_time, screen_was_locked);

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidEnterForeground() {
  ads_client_notifier_.NotifyBrowserDidEnterForeground();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidEnterBackground() {
  ads_client_notifier_.NotifyBrowserDidEnterBackground();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidBecomeActive() {
  ads_client_notifier_.NotifyBrowserDidBecomeActive();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyBrowserDidResignActive() {
  ads_client_notifier_.NotifyBrowserDidResignActive();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::NotifyDidSolveAdaptiveCaptcha() {
  ads_client_notifier_.NotifyDidSolveAdaptiveCaptcha();

  RunTaskEnvironmentUntilIdle();
}

void AdsClientNotifierForTesting::SimulateOpeningNewTab(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    int http_status_code) {
  CHECK(!redirect_chains_.contains(tab_id)) << "Tab already open";

  redirect_chains_[tab_id] = redirect_chain;

  SimulateSelectTab(tab_id);

  SimulateNavigateToURL(tab_id, redirect_chain, http_status_code);
}

void AdsClientNotifierForTesting::SimulateNavigateToURL(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    int http_status_code) {
  CHECK(redirect_chains_.contains(tab_id)) << "Tab does not exist";

  redirect_chains_[tab_id] = redirect_chain;

  const bool is_visible = tab_id == visible_tab_id_;

  NotifyTabDidChange(tab_id, redirect_chain, /*is_new_navigation=*/true,
                     /*is_restoring=*/false, is_visible);
  NotifyTabDidLoad(tab_id, http_status_code);
}

void AdsClientNotifierForTesting::SimulateSelectTab(int32_t tab_id) {
  CHECK(redirect_chains_.contains(tab_id)) << "Tab does not exist";

  if (visible_tab_id_) {
    // Occlude the previously visible tab.
    CHECK_NE(*visible_tab_id_, tab_id) << "Tab already selected";
    CHECK(redirect_chains_.contains(*visible_tab_id_));

    NotifyTabDidChange(*visible_tab_id_, redirect_chains_[*visible_tab_id_],
                       /*is_new_navigation=*/false, /*is_restoring=*/false,
                       /*is_visible=*/false);
  }
  visible_tab_id_ = tab_id;

  NotifyTabDidChange(tab_id, redirect_chains_[tab_id],
                     /*is_new_navigation=*/false, /*is_restoring=*/false,
                     /*is_visible=*/true);
}

void AdsClientNotifierForTesting::SimulateClosingTab(int32_t tab_id) {
  CHECK(redirect_chains_.contains(tab_id)) << "Tab does not exist";

  NotifyDidCloseTab(tab_id);

  redirect_chains_.erase(tab_id);

  const bool should_select_last_tab =
      tab_id == visible_tab_id_ && !redirect_chains_.empty();
  visible_tab_id_.reset();

  if (should_select_last_tab) {
    SimulateSelectLastTab();
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdsClientNotifierForTesting::SimulateSelectLastTab() {
  CHECK(!redirect_chains_.empty()) << "No tabs";

  const auto redirect_chain = redirect_chains_.crbegin();
  const auto [tab_id, _] = *redirect_chain;
  SimulateSelectTab(tab_id);
}

void AdsClientNotifierForTesting::RunTaskEnvironmentUntilIdle() {
  CHECK(task_environment_)
      << "set_ads_client_notifier_task_environment must be set before calling "
         "AdsClientNotifierForTesting::Notify* functions";
  task_environment_->RunUntilIdle();
}

}  // namespace brave_ads
