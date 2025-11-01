/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/brave_https_upgrade_policy_handler.h"

#include "brave/components/constants/pref_names.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

namespace policy {

namespace {

// Values for the DefaultBraveHttpsUpgradeSetting policy. These must be in sync
// with the policy definition DefaultBraveHttpsUpgradeSetting.yaml.
enum DefaultBraveHttpsUpgradeSetting {
  kDisabled = 1,
  kStrict = 2,
  kStandard = 3,
};

}  // namespace

BraveHttpsUpgradePolicyHandler::BraveHttpsUpgradePolicyHandler()
    : IntRangePolicyHandler(key::kDefaultBraveHttpsUpgradeSetting,
                            kManagedDefaultBraveHttpsUpgrade,
                            kDisabled,
                            kStandard,
                            /*clamp=*/false) {}

BraveHttpsUpgradePolicyHandler::~BraveHttpsUpgradePolicyHandler() = default;

}  // namespace policy
