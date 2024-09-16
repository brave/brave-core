/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_FOR_TESTING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_FOR_TESTING_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"

class GURL;

namespace brave_ads {

// A testing implementation of `AdsClientNotifier` designed to ensure that
// background tasks execute until there are no remaining tasks.

class AdsClientNotifierForTesting : public AdsClientNotifier {
 public:
  // Must be set before calling `Notify*` functions.
  void set_ads_client_notifier_task_environment(
      base::test::TaskEnvironment* task_environment) {
    CHECK(task_environment);

    task_environment_ = task_environment;
  }

  void NotifyDidInitializeAds() override;

  void NotifyLocaleDidChange(const std::string& locale) override;

  void NotifyPrefDidChange(const std::string& path) override;

  void NotifyResourceComponentDidChange(const std::string& manifest_version,
                                        const std::string& id) override;
  void NotifyDidUnregisterResourceComponent(const std::string& id) override;

  void NotifyRewardsWalletDidUpdate(
      const std::string& payment_id,
      const std::string& recovery_seed_base64) override;

  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) override;
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) override;
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_visible) override;
  void NotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void NotifyDidCloseTab(int32_t tab_id) override;

  void NotifyUserGestureEventTriggered(int32_t page_transition_type) override;

  void NotifyUserDidBecomeIdle() override;
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked) override;

  void NotifyBrowserDidEnterForeground() override;
  void NotifyBrowserDidEnterBackground() override;

  void NotifyBrowserDidBecomeActive() override;
  void NotifyBrowserDidResignActive() override;

  void NotifyDidSolveAdaptiveCaptcha() override;

 private:
  void RunTaskEnvironmentUntilIdle();

  raw_ptr<base::test::TaskEnvironment> task_environment_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_FOR_TESTING_H_
