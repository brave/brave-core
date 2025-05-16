/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_PROFILE_MODEL_PROFILE_MANAGER_IOS_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_PROFILE_MODEL_PROFILE_MANAGER_IOS_IMPL_H_

#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"

#define OnProfileCreationFinished                                            \
  OnProfileCreationFinished_ChromiumImpl(ProfileIOS* profile,                \
                                         CreationMode creation_mode,         \
                                         bool is_new_profile, bool success); \
  void OnProfileCreationFinished

#include "src/ios/chrome/browser/profile/model/profile_manager_ios_impl.h"  // IWYU pragma: export

#undef OnProfileCreationFinished

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_PROFILE_MODEL_PROFILE_MANAGER_IOS_IMPL_H_
