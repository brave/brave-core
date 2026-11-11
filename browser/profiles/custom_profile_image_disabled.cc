// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/gfx/image/image.h"

class ProfileAttributesEntry;

// No-op hooks for platforms without custom profile images.

gfx::Image BraveGetCustomProfileImage(const ProfileAttributesEntry&) {
  return {};
}

bool BraveIsCustomProfileImageSelected(const ProfileAttributesEntry&) {
  return false;
}

void BraveSelectFallbackProfileImage(ProfileAttributesEntry&) {}

void BraveRemoveCustomProfileImage(ProfileAttributesEntry&) {}
