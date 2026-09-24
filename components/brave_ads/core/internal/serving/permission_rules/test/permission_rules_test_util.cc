/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/permission_rules_test_util.h"

#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/catalog_permission_rule_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/confirmation_tokens_permission_rule_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/issuers_permission_rule_test_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/test/user_activity_permission_rule_test_util.h"

namespace brave_ads::test {

void ForcePermissionRules() {
  ForceCatalogPermissionRule();
  ForceConfirmationTokensPermissionRule();
  ForceIssuersPermissionRule();
  ForceUserActivityPermissionRule();
}

}  // namespace brave_ads::test
