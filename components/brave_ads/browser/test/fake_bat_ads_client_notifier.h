/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_CLIENT_NOTIFIER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_CLIENT_NOTIFIER_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "url/gurl.h"

namespace brave_ads::test {

// Records calls to `NotifyUserDidBecomeIdle` and `NotifyUserDidBecomeActive`
// so tests can assert on idle notification counts and arguments.
class FakeBatAdsClientNotifier : public bat_ads::mojom::BatAdsClientNotifier {
 public:
  FakeBatAdsClientNotifier();

  FakeBatAdsClientNotifier(const FakeBatAdsClientNotifier&) = delete;
  FakeBatAdsClientNotifier& operator=(const FakeBatAdsClientNotifier&) = delete;

  ~FakeBatAdsClientNotifier() override;

  void BindReceiver(mojo::PendingReceiver<bat_ads::mojom::BatAdsClientNotifier>
                        pending_receiver);

  size_t become_idle_count() const { return become_idle_count_; }
  size_t become_active_count() const { return become_active_count_; }
  base::TimeDelta last_idle_time() const { return last_idle_time_; }
  bool last_screen_was_locked() const { return last_screen_was_locked_; }

  // bat_ads::mojom::BatAdsClientNotifier:
  void NotifyDidInitializeAds() override {}
  void NotifyPrefDidChange(const std::string& /*path*/) override {}
  void NotifyResourceComponentDidChange(const std::string& /*manifest_version*/,
                                        const std::string& /*id*/) override {}
  void NotifyDidUnregisterResourceComponent(
      const std::string& /*id*/) override {}
  void NotifyRewardsWalletDidUpdate(
      const std::string& /*payment_id*/,
      const std::string& /*recovery_seed_base64*/) override {}
  void NotifyTabTextContentDidChange(
      int32_t /*tab_id*/,
      const std::vector<GURL>& /*redirect_chain*/,
      const std::string& /*text*/) override {}
  void NotifyTabDidStartPlayingMedia(int32_t /*tab_id*/) override {}
  void NotifyTabDidStopPlayingMedia(int32_t /*tab_id*/) override {}
  void NotifyTabDidChange(int32_t /*tab_id*/,
                          const std::vector<GURL>& /*redirect_chain*/,
                          bool /*is_new_navigation*/,
                          bool /*is_restoring*/,
                          bool /*is_visible*/) override {}
  void NotifyTabDidLoad(int32_t /*tab_id*/,
                        int32_t /*http_status_code*/) override {}
  void NotifyDidCloseTab(int32_t /*tab_id*/) override {}
  void NotifyUserGestureEventTriggered(int32_t /*page_transition*/) override {}
  void NotifyUserDidBecomeIdle() override;
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked) override;
  void NotifyBrowserDidEnterForeground() override {}
  void NotifyBrowserDidEnterBackground() override {}
  void NotifyBrowserDidBecomeActive() override {}
  void NotifyBrowserDidResignActive() override {}
  void NotifyDidSolveAdaptiveCaptcha() override {}

 private:
  size_t become_idle_count_ = 0;
  size_t become_active_count_ = 0;
  base::TimeDelta last_idle_time_;
  bool last_screen_was_locked_ = false;

  mojo::Receiver<bat_ads::mojom::BatAdsClientNotifier> receiver_{this};
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_CLIENT_NOTIFIER_H_
