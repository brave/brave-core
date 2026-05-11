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
  /* on the UI thread after the file write completes with `true` on        */ \
  /* success or `false` on failure.                                         */ \
  void SetBraveCustomAvatar(gfx::Image image,                                  \
                            base::OnceCallback<void(bool)> on_saved);          \
  /* Clears the user-uploaded custom profile avatar (also removes the      */ \
  /* file from disk on a background thread).                                */ \
  void ClearBraveCustomAvatar();                                               \
  /* Returns true when a user-uploaded custom profile avatar is set, which */ \
  /* takes precedence over GAIA and default avatars.                        */ \
  bool IsUsingBraveCustomAvatar() const;                                       \
                                                                               \
 private:                                                                      \
  friend class ProfileAttributeMigrationTest;                                  \
  void MigrateObsoleteProfileAttributes

#include <chrome/browser/profiles/profile_attributes_entry.h>  // IWYU pragma: export
#undef MigrateObsoleteProfileAttributes

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_
