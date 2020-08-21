/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/trials/trial.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"

namespace ads {

Trial::Trial(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Trial::~Trial() = default;

uint16_t Trial::GetGroup() {
  return group_;
}

///////////////////////////////////////////////////////////////////////////////

void Disable() {
  return;
}

}  // namespace ads
