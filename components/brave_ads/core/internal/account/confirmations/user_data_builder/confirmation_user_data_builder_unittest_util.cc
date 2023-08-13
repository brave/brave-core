/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"

namespace brave_ads {

namespace {

void MockDiagnosticId() {
  SetStringPref(prefs::kDiagnosticId, "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");
}

}  // namespace

void MockConfirmationUserData() {
  MockDiagnosticId();

  MockDeviceId();

  SetCatalogId(kCatalogId);
}

}  // namespace brave_ads
