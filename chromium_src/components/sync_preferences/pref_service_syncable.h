/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PREFERENCES_PREF_SERVICE_SYNCABLE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PREFERENCES_PREF_SERVICE_SYNCABLE_H_

#define CreateIncognitoPrefService                                   \
  CreateScopedPrefService(PrefStore* incognito_extension_pref_store, \
                          const std::vector<const char*>& prefix);   \
  std::unique_ptr<PrefServiceSyncable> CreateIncognitoPrefService

#include "src/components/sync_preferences/pref_service_syncable.h"

#undef CreateIncognitoPrefService

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PREFERENCES_PREF_SERVICE_SYNCABLE_H_
