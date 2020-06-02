/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_referrals/brave_referrals_service_factory.h"

#include <memory>

#include "brave/browser/brave_stats_updater_util.h"
#include "brave/components/brave_referrals/browser/brave_referrals_service.h"

namespace brave {

// static
std::unique_ptr<BraveReferralsService>
  BraveReferralsServiceFactory::GetForPrefs(
    PrefService* pref_service) {
  return std::make_unique<BraveReferralsService>(
      pref_service, GetAPIKey(), GetPlatformIdentifier());
}

// static
BraveReferralsServiceFactory* BraveReferralsServiceFactory::GetInstance() {
  return base::Singleton<BraveReferralsServiceFactory>::get();
}

BraveReferralsServiceFactory::BraveReferralsServiceFactory() {}

BraveReferralsServiceFactory::~BraveReferralsServiceFactory() {}

}  // namespace brave
