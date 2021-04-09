/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/components/tor/tor_constants.h"

// Do this for legacy tor profile migration because tor profile might be last
// active profile before upgrading
#define BRAVE_GET_LAST_USED_PROFILE_BASENAME                  \
  if (!last_used_profile_base_name.empty() &&                 \
      last_used_profile_base_name ==                          \
          base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe()) \
    return chrome::kInitialProfile;

#include "../../../../../chrome/browser/profiles/profile_manager.cc"

#undef BRAVE_GET_LAST_USED_PROFILE_BASENAME
