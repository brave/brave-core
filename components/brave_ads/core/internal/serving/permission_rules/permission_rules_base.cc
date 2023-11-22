/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules_base.h"

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

namespace brave_ads {

PermissionRulesBase::PermissionRulesBase() = default;

PermissionRulesBase::~PermissionRulesBase() = default;

// static
bool PermissionRulesBase::HasPermission() {
  if (!HasIssuersPermission()) {
    return false;
  }
  if (!HasConfirmationTokensPermission()) {
    return false;
  }
  if (!HasCommandLinePermission()) {
    return false;
  }

  return true;
}

}  // namespace brave_ads
