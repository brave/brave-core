/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads_observer.h"

namespace ads {

AdsObserver::AdsObserver() = default;

AdsObserver::~AdsObserver() = default;

mojo::PendingRemote<bat_ads::mojom::BatAdsObserver> AdsObserver::Bind() {
  Reset();
  return receiver_.BindNewPipeAndPassRemote();
}

}  // namespace ads
