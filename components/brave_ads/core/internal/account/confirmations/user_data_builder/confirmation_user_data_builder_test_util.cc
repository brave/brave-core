/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_test_util.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads::test {

namespace {

void MockDiagnosticId() {
  SetProfileStringPrefValue(prefs::kDiagnosticId, kDiagnosticId);
}

}  // namespace

void MockConfirmationUserData() {
  MockDiagnosticId();

  MockDeviceId();

  SetCatalogId(kCatalogId);
}

}  // namespace brave_ads::test
