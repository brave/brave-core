/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"

#define BRAVE_INIT                            \
  if (brave::IsSessionProfilePath(GetPath())) \
    brave::CreateParentProfileData(this);     \
  else

#include "../../../../../chrome/test/base/testing_profile.cc"
#undef BRAVE_INIT
