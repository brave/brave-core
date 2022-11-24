/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads_client_observer.h"

namespace ads {

AdsClientObserver::AdsClientObserver() = default;

AdsClientObserver::~AdsClientObserver() = default;

mojo::PendingRemote<bat_ads::mojom::BatAdsClientObserver>
AdsClientObserver::CreatePendingReceiverAndPassRemote() {
  Reset();
  return pending_receiver_.InitWithNewPipeAndPassRemote();
}

void AdsClientObserver::BindReceiver() {
  DCHECK(pending_receiver_.is_valid());
  receiver_.Bind(std::move(pending_receiver_));
}

}  // namespace ads
