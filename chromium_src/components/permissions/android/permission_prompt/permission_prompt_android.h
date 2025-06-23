/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_PROMPT_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_PROMPT_ANDROID_H_

#include "components/permissions/android/permission_prompt/permission_dialog_delegate.h"
#include "components/permissions/request_type.h"

#define PermissionPromptAndroid PermissionPromptAndroid_ChromiumImpl

#define PermissionCount                                                       \
  NotUsed() {                                                                 \
    return 0;                                                                 \
  }                                                                           \
  /* We can't override delegate to make it public, because at              */ \
  /* permission_prompt_android.h delegate is used both                     */ \
  /* as the argument name and the method name */                              \
  Delegate* delegate_public() const {                                         \
    return delegate_;                                                         \
  }                                                                           \
  /* Public setter for upstream's private permission_dialog_delegate_      */ \
  void set_permission_dialog_delegate(                                        \
      std::unique_ptr<PermissionDialogDelegate> permission_dialog_delegate) { \
    permission_dialog_delegate_ = std::move(permission_dialog_delegate);      \
  }                                                                           \
  size_t PermissionCount

#include <components/permissions/android/permission_prompt/permission_prompt_android.h>  // IWYU pragma: export

#undef PermissionCount
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

  void CreatePermissionDialogDelegate() {
    // This method does the same as upstream's
    // PermissionPromptAndroid::CreatePermissionDialogDelegate:
    // permission_dialog_delegate_ =
    //     PermissionDialogDelegate::Create(web_contents_, this);
    std::unique_ptr<PermissionDialogDelegate> permission_dialog_delegate =
        PermissionDialogDelegate::Create(web_contents(), this);
    set_permission_dialog_delegate(std::move(permission_dialog_delegate));
  }
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_PROMPT_ANDROID_H_
