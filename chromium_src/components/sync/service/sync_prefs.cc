/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Inject kAIChat handling into GetPrefNameForType,
// IsTypeSupportedInTransportMode, and IsTypeSelectedByDefaultInTransportMode.
#define BRAVE_SYNC_PREFS_GET_PREF_NAME_FOR_TYPE \
  case UserSelectableType::kAIChat:             \
    return prefs::internal::kSyncAIChat;

#define BRAVE_SYNC_PREFS_IS_TYPE_SUPPORTED_IN_TRANSPORT_MODE \
  case UserSelectableType::kAIChat:                          \
    return true;

#define BRAVE_SYNC_PREFS_IS_TYPE_SELECTED_BY_DEFAULT \
  case UserSelectableType::kAIChat:                  \
    return true;

#include <components/sync/service/sync_prefs.cc>

#undef BRAVE_SYNC_PREFS_IS_TYPE_SELECTED_BY_DEFAULT
#undef BRAVE_SYNC_PREFS_IS_TYPE_SUPPORTED_IN_TRANSPORT_MODE
#undef BRAVE_SYNC_PREFS_GET_PREF_NAME_FOR_TYPE
