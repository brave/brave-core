/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/ads_service.h"

#include <utility>

namespace brave_ads {

AdsService::AdsService(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {}

AdsService::~AdsService() = default;

}  // namespace brave_ads
