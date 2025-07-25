/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_PREFS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_PREFS_H_

#define GetSelectedTypesForAccount(...)                       \
  GetSelectedTypesForAccount_ChromiumImpl(__VA_ARGS__) const; \
  UserSelectableTypeSet GetSelectedTypesForAccount(__VA_ARGS__)
#define GetSelectedTypesForSyncingUser(...)                       \
  GetSelectedTypesForSyncingUser_ChromiumImpl(__VA_ARGS__) const; \
  UserSelectableTypeSet GetSelectedTypesForSyncingUser(__VA_ARGS__)

#include <components/sync/service/sync_prefs.h>  // IWYU pragma: export

#undef GetSelectedTypesForAccount
#undef GetSelectedTypesForSyncingUser

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_PREFS_H_
