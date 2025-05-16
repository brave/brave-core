// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// These overrides temporarily restore the ability to create a Profile
// synchronously

#include "ios/chrome/browser/profile/model/profile_manager_ios_impl.h"

#include "ios/chrome/browser/shared/model/profile/profile_attributes_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"

#define kAsynchronous kSynchronous
#define GetProfileName                                   \
  GetProfileName();                                      \
  if (profiles_map_.find(name) == profiles_map_.end()) { \
    return;                                              \
  }                                                      \
  if (false)                                             \
  auto unused = profile->GetProfileName
#define BRAVE_CREATE_OR_LOAD_PROFILE                                       \
  if (!inserted && !profile_info.is_loaded()) {                            \
    return false;                                                          \
  }                                                                        \
  if (inserted) {                                                          \
    OnProfileCreationFinished(profile_info.profile(),                      \
                              CreationMode::kAsynchronous, is_new_profile, \
                              /* success */ true);                         \
  }
#include "src/ios/chrome/browser/profile/model/profile_manager_ios_impl.mm"
#undef BRAVE_CREATE_OR_LOAD_PROFILE
#undef GetProfileName
#undef kAsynchronous
