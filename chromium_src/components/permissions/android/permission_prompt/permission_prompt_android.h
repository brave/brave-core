/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_PROMPT_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_PROMPT_ANDROID_H_

#include "components/permissions/request_type.h"

#define PermissionPromptAndroid PermissionPromptAndroid_ChromiumImpl
#define GetIconId virtual GetIconId
#define ShouldUseRequestingOriginFavicon \
  virtual ShouldUseRequestingOriginFavicon

#define PermissionCount                            \
  NotUsed() { return 0; }                          \
  Delegate* delegate() const { return delegate_; } \
  size_t PermissionCount

#include "src/components/permissions/android/permission_prompt/permission_prompt_android.h"  // IWYU pragma: export

#undef PermissionCount
#undef ShouldUseRequestingOriginFavicon
#undef GetIconId
#undef PermissionPromptAndroid

namespace permissions {

class PermissionPromptAndroid : public PermissionPromptAndroid_ChromiumImpl {
 public:
  using PermissionPromptAndroid_ChromiumImpl::
      PermissionPromptAndroid_ChromiumImpl;

  PermissionPromptAndroid(const PermissionPromptAndroid&) = delete;
  PermissionPromptAndroid& operator=(const PermissionPromptAndroid&) = delete;

  ~PermissionPromptAndroid() override = default;

  int GetIconId() const override;
  bool ShouldUseRequestingOriginFavicon() const override;
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_PROMPT_ANDROID_H_
