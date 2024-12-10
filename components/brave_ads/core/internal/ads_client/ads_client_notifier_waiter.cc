/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_waiter.h"

#include <utility>

#include "base/run_loop.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"

namespace brave_ads::test {

AdsClientNotifierWaiter::AdsClientNotifierWaiter(AdsClientNotifier& notifier)
    : notifier_(notifier) {
  notifier_->AddObserver(this);
}

AdsClientNotifierWaiter::~AdsClientNotifierWaiter() {
  notifier_->RemoveObserver(this);
}

void AdsClientNotifierWaiter::WaitForAdsInitialization() {
  base::RunLoop run_loop;
  did_initialize_ads_closure_ = run_loop.QuitClosure();
  run_loop.Run();
}

void AdsClientNotifierWaiter::OnNotifyDidInitializeAds() {
  if (did_initialize_ads_closure_) {
    std::move(did_initialize_ads_closure_).Run();
  }
}

}  // namespace brave_ads::test
