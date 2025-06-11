/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_USER_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_USER_SETTINGS_H_

// We need to expand SyncUserSettings with
// KeepAccountSettingsPrefsOnlyForUsers_Unused method to match override at
// SyncServiceAndroidBridge::KeepAccountSettingsPrefsOnlyForUsers because it now
// call internally
// GetUserSettings()->KeepAccountSettingsPrefsOnlyForUsers_Unused(gaia_id_hashes);

#define KeepAccountSettingsPrefsOnlyForUsers            \
  KeepAccountSettingsPrefsOnlyForUsers_Unused(          \
      const std::vector<GaiaId>& available_gaia_ids) {} \
  virtual void KeepAccountSettingsPrefsOnlyForUsers

#include "src/components/sync/service/sync_user_settings.h"  // IWYU pragma: export

#undef KeepAccountSettingsPrefsOnlyForUsers

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_USER_SETTINGS_H_
