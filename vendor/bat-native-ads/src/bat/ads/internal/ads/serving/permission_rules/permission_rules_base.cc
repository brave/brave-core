/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/permission_rules_base.h"

#include "bat/ads/internal/ads/serving/permission_rules/command_line_permission_rule.h"
#include "bat/ads/internal/ads/serving/permission_rules/issuers_permission_rule.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/unblinded_tokens_permission_rule.h"

namespace ads {

PermissionRulesBase::PermissionRulesBase() = default;

PermissionRulesBase::~PermissionRulesBase() = default;

// static
bool PermissionRulesBase::HasPermission() {
  IssuersPermissionRule issuers_permission_rule;
  if (!ShouldAllow(&issuers_permission_rule)) {
    return false;
  }

  UnblindedTokensPermissionRule unblinded_tokens_permission_rule;
  if (!ShouldAllow(&unblinded_tokens_permission_rule)) {
    return false;
  }

  CommandLinePermissionRule catalog_permission_rule;
  return ShouldAllow(&catalog_permission_rule);
}

}  // namespace ads
