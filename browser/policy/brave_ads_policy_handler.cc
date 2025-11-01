/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/brave_ads_policy_handler.h"

#include "brave/components/constants/pref_names.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

namespace policy {

namespace {

// Values for the DefaultBraveAdsSetting policy. These must be in sync
// with the policy definition DefaultBraveAdsSetting.yaml.
enum DefaultBraveAdsSetting {
  kAllowAds = 1,
  kBlockAds = 2,
};

}  // namespace

BraveAdsPolicyHandler::BraveAdsPolicyHandler()
    : IntRangePolicyHandler(key::kDefaultBraveAdsSetting,
                            kManagedDefaultBraveAds,
                            kAllowAds,
                            kBlockAds,
                            /*clamp=*/false) {}

BraveAdsPolicyHandler::~BraveAdsPolicyHandler() = default;

}  // namespace policy
