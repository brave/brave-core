/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/permission_rules_base.h"

#include "bat/ads/internal/frequency_capping/permission_rules/catalog_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/issuers_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"

namespace ads {

PermissionRulesBase::PermissionRulesBase() = default;

PermissionRulesBase::~PermissionRulesBase() = default;

bool PermissionRulesBase::HasPermission() const {
  CatalogFrequencyCap catalog_frequency_cap;
  if (!ShouldAllow(&catalog_frequency_cap)) {
    return false;
  }

  IssuersFrequencyCap issuers_frequency_cap;
  if (!ShouldAllow(&issuers_frequency_cap)) {
    return false;
  }

  UnblindedTokensFrequencyCap unblinded_tokens_frequency_cap;
  if (!ShouldAllow(&unblinded_tokens_frequency_cap)) {
    return false;
  }

  UserActivityFrequencyCap user_activity_frequency_cap;
  if (!ShouldAllow(&user_activity_frequency_cap)) {
    return false;
  }

  return true;
}

}  // namespace ads
