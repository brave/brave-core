/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_attributes_entry.h"

#include "chrome/browser/profiles/profile_avatar_icon_util.h"

void ProfileAttributesEntry::BraveMigrateObsoleteProfileAttributes() {
  // Run our migrations
#if !BUILDFLAG(IS_ANDROID)
  const int kPlaceholderAvatarIndex =
      static_cast<int>(profiles::GetPlaceholderAvatarIndex());

  const int kBraveAvatarIconStartIndex =
      static_cast<int>(profiles::GetDefaultAvatarIconCount() -
                       profiles::kBraveDefaultAvatarIconsCount);
  // Added 25 July 2024
  // see https://github.com/brave/brave-browser/issues/40005
  //
  // Brave originally allowed folks to pick the Chromium profile icons.
  // We then removed those in favor of our own branded icons in 0.70.x (2019).
  // https://github.com/brave/brave-core/pull/3165
  //
  // The old ones would continue to work - but may have had rendering issues.
  // Chromium 127 had a Windows change which now triggers a CHECK.
  //
  // This migration moves folks who have the old IDs to the default profile ID.
  int icon_index = GetAvatarIconIndex();
  if (icon_index < kBraveAvatarIconStartIndex &&
      icon_index != kPlaceholderAvatarIndex) {
    SetAvatarIconIndex(kPlaceholderAvatarIndex);
  }
#endif
}

#define BRAVE_PROFILE_ATTRIBUTES_ENTRY_MIGRATE_OBSOLETE_PROFILE_ATTRIBUTES \
  BraveMigrateObsoleteProfileAttributes();

#include "src/chrome/browser/profiles/profile_attributes_entry.cc"
#undef BRAVE_PROFILE_ATTRIBUTES_ENTRY_MIGRATE_OBSOLETE_PROFILE_ATTRIBUTES
