/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/services/bat_ads_service_launch.h"

#include <memory>
#include <utility>

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"

namespace brave_ads {

BatAdsServiceLaunch::BatAdsServiceLaunch() {
  // Constructed on the UI thread, but `Bind` and `Cancel` always run on the
  // same dedicated `SingleThreadTaskRunner`, so bind the checker lazily to
  // that sequence instead of the construction sequence.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

BatAdsServiceLaunch::~BatAdsServiceLaunch() = default;

void BatAdsServiceLaunch::Bind(
    mojo::PendingReceiver<bat_ads::mojom::BatAdsService>
        bat_ads_service_pending_receiver) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (cancelled_) {
    return;
  }

  receiver_ = mojo::MakeSelfOwnedReceiver(
      std::make_unique<bat_ads::BatAdsServiceImpl>(),
      std::move(bat_ads_service_pending_receiver));
}

void BatAdsServiceLaunch::Cancel() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  cancelled_ = true;

  if (receiver_) {
    receiver_->Close();
    receiver_.reset();
  }
}

}  // namespace brave_ads
