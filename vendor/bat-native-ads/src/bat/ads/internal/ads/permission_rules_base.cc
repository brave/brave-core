/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/permission_rules_base.h"

#include "bat/ads/internal/frequency_capping/permission_rules/catalog_permission_rule.h"
#include "bat/ads/internal/frequency_capping/permission_rules/issuers_permission_rule.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_permission_rule.h"

namespace ads {

PermissionRulesBase::PermissionRulesBase() = default;

PermissionRulesBase::~PermissionRulesBase() = default;

bool PermissionRulesBase::HasPermission() const {
  CatalogPermissionRule catalog_permission_rule;
  if (!ShouldAllow(&catalog_permission_rule)) {
    return false;
  }

  IssuersPermissionRule issuers_permission_rule;
  if (!ShouldAllow(&issuers_permission_rule)) {
    return false;
  }

  UnblindedTokensPermissionRule unblinded_tokens_permission_rule;
  if (!ShouldAllow(&unblinded_tokens_permission_rule)) {
    return false;
  }

  return true;
}

}  // namespace ads
