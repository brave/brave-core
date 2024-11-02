/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads.h"

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator.h"
#include "brave/components/brave_ads/core/internal/ads_impl.h"

namespace brave_ads {

// static
std::unique_ptr<Ads> Ads::CreateInstance(AdsClient& ads_client) {
  return std::make_unique<AdsImpl>(ads_client,
                                   std::make_unique<TokenGenerator>());
}

}  // namespace brave_ads
