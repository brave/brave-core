/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/catalog_permission_rule_unittest_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

namespace ads {

void ForceCatalogPermissionRuleForTesting() {
  SetCatalogId("573c74fa-623a-4a46-adce-e688dfb7e8f5");
  SetCatalogVersion(1);
  SetCatalogPing(base::Hours(2));
  SetCatalogLastUpdated(Now());
}

}  // namespace ads
