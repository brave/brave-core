/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_destroyer.h"

// When there are multiple OTR profiles, such as with a Private window and a Tor
// window open, it doesn't make sense that when destroying one of the OTR
// profiles the Chromium code wants to check that hosts for all OTR profiles are
// gone.
#define BRAVE_PROFILE_DESTROYER_DESTROY_PROFILE_WHEN_APPROPRIATE \
  if (!profile->IsOffTheRecord())

#include "src/chrome/browser/profiles/profile_destroyer.cc"
#undef BRAVE_PROFILE_DESTROYER_DESTROY_PROFILE_WHEN_APPROPRIATE
