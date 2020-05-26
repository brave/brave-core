/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define StoresDataForNonSyncingPrimaryAccountInAccountDB \
  DISABLED_StoresDataForNonSyncingPrimaryAccountInAccountDB
#define StoresDataForSecondaryAccountInAccountDB \
  DISABLED_StoresDataForSecondaryAccountInAccountDB
#define ClearsAccountDBOnSignout DISABLED_ClearsAccountDBOnSignout
#define SwitchesStoresOnEnablingSync DISABLED_SwitchesStoresOnEnablingSync
#define SwitchesStoresOnMakingAccountPrimary \
  DISABLED_SwitchesStoresOnMakingAccountPrimary
#include "../../../../../../../chrome/browser/sync/test/integration/single_client_passwords_sync_test.cc"
#undef StoresDataForNonSyncingPrimaryAccountInAccountDB
#undef StoresDataForSecondaryAccountInAccountDB
#undef ClearsAccountDBOnSignout
#undef SwitchesStoresOnEnablingSync
#undef SwitchesStoresOnMakingAccountPrimary
