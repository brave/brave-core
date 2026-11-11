// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_HOOKS_H_
#define BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_HOOKS_H_

#include "build/build_config.h"
#include "ui/gfx/image/image.h"

class ProfileAttributesEntry;

// Called from upstream profile code, which cannot depend on the buildflag
// header. profile_image.cc asserts that this platform check matches
// BUILDFLAG(ENABLE_CUSTOM_PROFILE_IMAGE).
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
// The active custom image, or an empty image while it loads or when none is
// active.
gfx::Image BraveGetCustomProfileImage(const ProfileAttributesEntry& entry);
// Whether the custom image, not a preset, is the profile's selection.
bool BraveIsCustomProfileImageSelected(const ProfileAttributesEntry& entry);
// Records that the user chose a preset avatar.
void BraveSelectFallbackProfileImage(ProfileAttributesEntry& entry);
// Deletes the image with its profile, after already accepted writes.
void BraveRemoveCustomProfileImage(ProfileAttributesEntry& entry);
#else
inline gfx::Image BraveGetCustomProfileImage(const ProfileAttributesEntry&) {
  return {};
}
inline bool BraveIsCustomProfileImageSelected(const ProfileAttributesEntry&) {
  return false;
}
inline void BraveSelectFallbackProfileImage(ProfileAttributesEntry&) {}
inline void BraveRemoveCustomProfileImage(ProfileAttributesEntry&) {}
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)

#endif  // BRAVE_BROWSER_PROFILES_CUSTOM_PROFILE_IMAGE_HOOKS_H_
