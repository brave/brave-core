/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/brave_remember_1p_storage_policy_handler.h"

#include "brave/components/constants/pref_names.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

namespace policy {

namespace {

// Values for the DefaultBraveRemember1PStorageSetting policy. These must be in
// sync with the policy definition DefaultBraveRemember1PStorageSetting.yaml.
enum DefaultBraveRemember1PStorageSetting {
  kRememberFirstPartyStorage = 1,
  kForgetFirstPartyStorage = 2,
};

}  // namespace

BraveRemember1PStoragePolicyHandler::BraveRemember1PStoragePolicyHandler()
    : IntRangePolicyHandler(key::kDefaultBraveRemember1PStorageSetting,
                            kManagedDefaultBraveRemember1PStorage,
                            kRememberFirstPartyStorage,
                            kForgetFirstPartyStorage,
                            /*clamp=*/false) {}

BraveRemember1PStoragePolicyHandler::~BraveRemember1PStoragePolicyHandler() =
    default;

}  // namespace policy
