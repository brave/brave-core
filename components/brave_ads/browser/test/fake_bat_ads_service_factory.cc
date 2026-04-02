/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_bat_ads_service_factory.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/browser/test/fake_bat_ads_service.h"

namespace brave_ads::test {

FakeBatAdsServiceFactory::FakeBatAdsServiceFactory() = default;

FakeBatAdsServiceFactory::~FakeBatAdsServiceFactory() = default;

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
