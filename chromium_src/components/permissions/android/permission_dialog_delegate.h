/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_DIALOG_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_DIALOG_DELEGATE_H_

#include "brave/components/permissions/permission_lifetime_utils.h"

#define BRAVE_PERMISSION_DIALOG_DELEGATE_H_                                 \
 private:                                                                   \
  void CreateAndSetLifetimeOptions();                                       \
  void ApplyLifetimeToPermissionRequests(JNIEnv* env,                       \
                                         const JavaParamRef<jobject>& obj); \
                                                                            \
  std::vector<PermissionLifetimeOption> lifetime_options_;

#include "../../../../../components/permissions/android/permission_dialog_delegate.h"

#undef BRAVE_PERMISSION_DIALOG_DELEGATE_H_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_DIALOG_DELEGATE_H_
