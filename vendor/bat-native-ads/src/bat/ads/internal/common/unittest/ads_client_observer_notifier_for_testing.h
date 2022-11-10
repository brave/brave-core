/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_ADS_CLIENT_OBSERVER_NOTIFIER_FOR_TESTING_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_ADS_CLIENT_OBSERVER_NOTIFIER_FOR_TESTING_H_

#include "bat/ads/ads_client_observer_notifier.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ads {

class AdsClientObserverNotifierForTesting : public AdsClientObserverNotifier {
 public:
  // AdsClientObserverNotifier:
  void NotifyLocaleDidChange(const std::string& locale);

  void NotifyPrefDidChange(const std::string& path);

  void NotifyDidUpdateResourceComponent(const std::string& id);

  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text);
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html);
  void NotifyTabDidStartPlayingMedia(int32_t tab_id);
  void NotifyTabDidStopPlayingMedia(int32_t tab_id);
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_visible,
                          bool is_incognito);
  void NotifyDidCloseTab(int32_t tab_id);

  void NotifyUserDidBecomeIdle();
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked);

  void NotifyBrowserDidEnterForeground();
  void NotifyBrowserDidEnterBackground();
  void NotifyBrowserDidBecomeActive();
  void NotifyBrowserDidResignActive();

  void NotifyRewardsWalletIsReady(const std::string& payment_id,
                                  const std::string& recovery_seed);
  void NotifyRewardsWalletDidChange(const std::string& payment_id,
                                    const std::string& recovery_seed);

 private:
  void FlushObserversForTesting();
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_ADS_CLIENT_OBSERVER_NOTIFIER_FOR_TESTING_H_
