/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_DIALOG_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_DIALOG_DELEGATE_H_

// Forward includes to avoid redefine of Create term
#include "base/task/cancelable_task_tracker.h"
#include "components/permissions/permission_util.h"
#include "content/public/browser/web_contents_observer.h"
// Forward includes to avoid redefine of CreateDialog term
#include "components/permissions/android/permission_prompt/permission_dialog_controller.h"

namespace permissions {
class PermissionPromptAndroid_ChromiumImpl;
}  // namespace permissions

#define OnRequestingOriginFaviconLoaded                                     \
  ApplyLifetimeToPermissionRequests(                                        \
      JNIEnv* env, PermissionPromptAndroid* permission_prompt);             \
  void ApplyDontAskAgainOption(JNIEnv* env,                                 \
                               PermissionPromptAndroid* permission_prompt); \
  void OnRequestingOriginFaviconLoaded

// Additional PermissionDialogDelegate::Create stub method to be in accordance
// with the overridden method at permission_prompt_android.h
//   void CreatePermissionDialogDelegate() {
//     permission_dialog_delegate_ =
//         PermissionDialogDelegate::Create(web_contents_, this);
//   }
// which after PermissionPromptAndroid_ChromiumImpl override wants us to pass
// PermissionPromptAndroid_ChromiumImpl* as the second arg.

#define Create                                                     \
  Create(content::WebContents* web_contents,                       \
         PermissionPromptAndroid_ChromiumImpl* permission_prompt); \
  static std::unique_ptr<PermissionDialogDelegate> Create

#define CreateDialog                 \
  BravePreCreateDialog(JNIEnv* env); \
  void CreateDialog

#include <components/permissions/android/permission_prompt/permission_dialog_delegate.h>  // IWYU pragma: export

#undef Create
#undef CreateDialog

#undef OnRequestingOriginFaviconLoaded

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_ANDROID_PERMISSION_PROMPT_PERMISSION_DIALOG_DELEGATE_H_
