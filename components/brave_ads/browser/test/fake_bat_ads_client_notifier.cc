/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_bat_ads_client_notifier.h"

#include <utility>

namespace brave_ads::test {

FakeBatAdsClientNotifier::FakeBatAdsClientNotifier() = default;

FakeBatAdsClientNotifier::~FakeBatAdsClientNotifier() = default;

void FakeBatAdsClientNotifier::BindReceiver(
    mojo::PendingReceiver<bat_ads::mojom::BatAdsClientNotifier>
        pending_receiver) {
  receiver_.Bind(std::move(pending_receiver));
}

void FakeBatAdsClientNotifier::NotifyUserDidBecomeIdle() {
  ++become_idle_count_;
}

void FakeBatAdsClientNotifier::NotifyUserDidBecomeActive(
    base::TimeDelta idle_time,
    bool screen_was_locked) {
  ++become_active_count_;
  last_idle_time_ = idle_time;
  last_screen_was_locked_ = screen_was_locked;
}

}  // namespace brave_ads::test
