/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_bat_ads_service_factory.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/browser/test/fake_bat_ads_client_notifier.h"
#include "brave/components/brave_ads/browser/test/fake_bat_ads_service.h"

namespace brave_ads::test {

FakeBatAdsServiceFactory::FakeBatAdsServiceFactory() = default;

FakeBatAdsServiceFactory::~FakeBatAdsServiceFactory() = default;

const FakeBatAdsClientNotifier*
FakeBatAdsServiceFactory::bat_ads_client_notifier() const {
  return bat_ads_service_ ? bat_ads_service_->bat_ads_client_notifier()
                          : nullptr;
}

size_t FakeBatAdsServiceFactory::become_idle_count() const {
  const FakeBatAdsClientNotifier* const notifier = bat_ads_client_notifier();
  return notifier ? notifier->become_idle_count() : 0U;
}

size_t FakeBatAdsServiceFactory::become_active_count() const {
  const FakeBatAdsClientNotifier* const notifier = bat_ads_client_notifier();
  return notifier ? notifier->become_active_count() : 0U;
}

base::TimeDelta FakeBatAdsServiceFactory::last_idle_time() const {
  const FakeBatAdsClientNotifier* const notifier = bat_ads_client_notifier();
  return notifier ? notifier->last_idle_time() : base::TimeDelta{};
}

bool FakeBatAdsServiceFactory::last_screen_was_locked() const {
  const FakeBatAdsClientNotifier* const notifier = bat_ads_client_notifier();
  return notifier ? notifier->last_screen_was_locked() : false;
}

mojo::Remote<bat_ads::mojom::BatAdsService> FakeBatAdsServiceFactory::Launch()
    const {
  ++launch_count_;

  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service_remote;

  bat_ads_service_ = std::make_unique<FakeBatAdsService>(
      base::BindRepeating(&FakeBatAdsServiceFactory::OnInitialize,
                          base::Unretained(this)),
      base::BindRepeating(&FakeBatAdsServiceFactory::OnShutdown,
                          base::Unretained(this)),
      simulate_initialization_failure_);
  bat_ads_service_->BindReceiver(
      bat_ads_service_remote.BindNewPipeAndPassReceiver());

  return bat_ads_service_remote;
}

}  // namespace brave_ads::test
