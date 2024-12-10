/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_WAITER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_WAITER_H_

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace brave_ads {
class AdsClientNotifier;
}  // namespace brave_ads

namespace brave_ads::test {

class AdsClientNotifierWaiter : public AdsClientNotifierObserver {
 public:
  explicit AdsClientNotifierWaiter(AdsClientNotifier& notifier);

  ~AdsClientNotifierWaiter() override;

  // AdsClientNotifierObserver overrides:
  void OnNotifyDidInitializeAds() override;

  void WaitForAdsInitialization();

 private:
  const base::raw_ref<AdsClientNotifier> notifier_;
  base::OnceClosure did_initialize_ads_closure_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_WAITER_H_
