/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/sync/service/sync_prefs.h"

#define GetSelectedTypesForAccount GetSelectedTypesForAccount_ChromiumImpl
#define GetSelectedTypesForSyncingUser \
  GetSelectedTypesForSyncingUser_ChromiumImpl

#include <components/sync/service/sync_prefs.cc>

#undef GetSelectedTypesForAccount
#undef GetSelectedTypesForSyncingUser

namespace syncer {

UserSelectableTypeSet SyncPrefs::GetSelectedTypesForAccount(
    const GaiaId& gaia_id) const {
  UserSelectableTypeSet selected_types =
      GetSelectedTypesForAccount_ChromiumImpl(gaia_id);
  selected_types.Remove(UserSelectableType::kPasswords);
  return selected_types;
}

UserSelectableTypeSet SyncPrefs::GetSelectedTypesForSyncingUser() const {
  UserSelectableTypeSet selected_types =
      GetSelectedTypesForSyncingUser_ChromiumImpl();
  selected_types.Remove(UserSelectableType::kPasswords);
  return selected_types;
}

}  // namespace syncer
