/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_ANDROID_H_

#define PermissionCount                            \
  NotUsed() { return 0; }                          \
  Delegate* delegate() const { return delegate_; } \
  size_t PermissionCount

#include "../../../../../components/permissions/android/permission_prompt_android.h"

#undef PermissionCount

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_ANDROID_H_
