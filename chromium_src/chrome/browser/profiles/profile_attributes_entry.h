/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_

#include "base/functional/callback_forward.h"
#include "build/build_config.h"

namespace base {
class DictValue;
}  // namespace base

#if BUILDFLAG(IS_WIN)
// To avoid conflicts with the macro from the Windows SDK...
#undef GetUserName
#endif

#define MigrateObsoleteProfileAttributes                                       \
  BraveMigrateObsoleteProfileAttributes();                                     \
                                                                               \
 public:                                                                       \
  const base::DictValue* GetSerpMetrics() const;                               \
  void SetSerpMetrics(base::DictValue serp_metrics);                           \
                                                                               \
  /* Returns the user-uploaded custom profile avatar image, or nullptr if   */ \
  /* none is set or the image has not finished loading from disk yet.       */ \
  const gfx::Image* GetBraveCustomAvatar() const;                              \
  /* Saves a user-uploaded custom profile avatar image. `on_saved` is run   */ \
  /* on the UI thread once the disk write has been scheduled (the file is   */ \
  /* persisted on a background thread). `true` means the write was          */ \
  /* accepted and the in-memory cache was already updated, so the next      */ \
  /* `GetBraveCustomAvatar()` call returns the new image immediately;       */ \
  /* `false` means the supplied image was empty and nothing was saved.      */ \
  void SetBraveCustomAvatar(gfx::Image image,                                  \
                            base::OnceCallback<void(bool)> on_saved);          \
  /* Clears the user-uploaded custom profile avatar (also removes the      */  \
  /* file from disk on a background thread).                                */ \
  void ClearBraveCustomAvatar();                                               \
  /* True when a custom avatar file is saved for this profile (may be     */   \
  /* inactive while the user uses a preset).                               */  \
  bool HasBraveCustomAvatar() const;                                           \
  /* True when the saved custom avatar is the one shown in Chrome UI      */   \
  /* (toolbar, menus, settings).                                            */ \
  bool IsUsingBraveCustomAvatar() const;                                       \
  /* Stops using the custom image for the profile icon; keeps the file.     */ \
  void DeactivateBraveCustomAvatar();                                          \
  /* Uses the saved custom image again for the profile icon.                */ \
  void ActivateBraveCustomAvatar();                                            \
                                                                               \
 private:                                                                      \
  friend class ProfileAttributeMigrationTest;                                  \
  void MigrateObsoleteProfileAttributes

#include <chrome/browser/profiles/profile_attributes_entry.h>  // IWYU pragma: export
#undef MigrateObsoleteProfileAttributes

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_
