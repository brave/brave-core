/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/services/bat_ads_service_launch.h"

#include <memory>
#include <utility>

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"

namespace brave_ads {

BatAdsServiceLaunch::BatAdsServiceLaunch() = default;

BatAdsServiceLaunch::~BatAdsServiceLaunch() = default;

void BatAdsServiceLaunch::Bind(
    mojo::PendingReceiver<bat_ads::mojom::BatAdsService>
        bat_ads_service_pending_receiver) {
  if (cancelled_) {
    return;
  }

  receiver_ = mojo::MakeSelfOwnedReceiver(
      std::make_unique<bat_ads::BatAdsServiceImpl>(),
      std::move(bat_ads_service_pending_receiver));
}

void BatAdsServiceLaunch::Cancel() {
  cancelled_ = true;

  if (receiver_) {
    receiver_->Close();
    receiver_.reset();
  }
}

}  // namespace brave_ads
