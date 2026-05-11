/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_manager.h"

// CHROMIUM_SRC_NOLINT: expanded in #included
// settings_manage_profile_handler.cc.
#define BRAVE_HANDLE_SET_PROFILE_ICON_TO_GAIA_AVATAR_END(profile) \
  do {                                                            \
    ProfileAttributesEntry* brave_entry =                         \
        g_browser_process->profile_manager()                      \
            ->GetProfileAttributesStorage()                       \
            .GetProfileAttributesWithPath((profile)->GetPath());  \
    if (brave_entry) {                                            \
      brave_entry->DeactivateBraveCustomAvatar();                 \
    }                                                             \
  } while (0)

#include <chrome/browser/ui/webui/settings/settings_manage_profile_handler.cc>

#undef BRAVE_HANDLE_SET_PROFILE_ICON_TO_GAIA_AVATAR_END
