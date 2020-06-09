/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_

#include "components/prefs/pref_change_registrar.h"
// Header guard to prevent Initialize from getting overriden in it
// ============================================================================
#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/sync_service_crypto.h"
// ============================================================================


#define BRAVE_PROFILE_SYNC_SERVICE_H_                                     \
 private:                                                                 \
  PrefChangeRegistrar brave_sync_prefs_change_registrar_;

#define Initialize \
  OnBraveSyncPrefsChanged(const std::string& path); \
  void Initialize

#include "../../../../../components/sync/driver/profile_sync_service.h"

#undef BRAVE_PROFILE_SYNC_SERVICE_H_
#undef Initialize
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_
