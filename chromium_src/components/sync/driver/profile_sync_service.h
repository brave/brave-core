/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_
#include "components/prefs/pref_change_registrar.h"


#define BRAVE_PROFILE_SYNC_SERVICE_H_                                     \
  void OnBraveSyncPrefsChanged(const std::string& path);                  \
                                                                          \
 private:                                                                 \
  PrefChangeRegistrar brave_sync_prefs_change_registrar_;

#include "../../../../../components/sync/driver/profile_sync_service.h"

#undef BRAVE_PROFILE_SYNC_SERVICE_H_
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_H_
