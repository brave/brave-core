/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/permissions/android/permission_prompt/permission_dialog_delegate.h"

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/check.h"
#include "base/notreached.h"
#include "brave/components/permissions/android/jni_headers/BravePermissionDialogDelegate_jni.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "brave/components/permissions/permission_widevine_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/android/permission_prompt/permission_prompt_android.h"
#include "components/permissions/features.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/l10n/l10n_util.h"

namespace permissions {
namespace {

// This method doesn't belong to PermissionDialogJavaDelegate because
// when it is invoked, PermissionDialogJavaDelegate::j_delegate_ is not ready
// yet
void SetLifetimeOptions(const base::android::JavaRef<jobject>& j_delegate) {
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BravePermissionDialogDelegate_setLifetimeOptionsText(
      env, j_delegate,
      base::android::ConvertUTF16ToJavaString(
          env, l10n_util::GetStringUTF16(
                   IDS_PERMISSIONS_BUBBLE_LIFETIME_COMBOBOX_LABEL)));

  std::vector<PermissionLifetimeOption> lifetime_options =
      CreatePermissionLifetimeOptions();
  std::vector<std::u16string> lifetime_labels;
  for (const auto& lifetime_option : lifetime_options) {
    lifetime_labels.push_back(lifetime_option.label);
  }

  Java_BravePermissionDialogDelegate_setLifetimeOptions(
      env, j_delegate,
      base::android::ToJavaArrayOfStrings(env, lifetime_labels));
}

}  // namespace

void PermissionDialogJavaDelegate::ApplyLifetimeToPermissionRequests(
    JNIEnv* env,
    PermissionPromptAndroid* permission_prompt) {
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return;
  }
  const int selected_lifetime_option =
      Java_BravePermissionDialogDelegate_getSelectedLifetimeOption(env,
                                                                   j_delegate_);
  DCHECK(!ShouldShowLifetimeOptions(permission_prompt->delegate_public()) ||
         selected_lifetime_option != -1);
  if (selected_lifetime_option != -1) {
    std::vector<PermissionLifetimeOption> lifetime_options =
        CreatePermissionLifetimeOptions();
    SetRequestsLifetime(lifetime_options, selected_lifetime_option,
                        permission_prompt->delegate_public());
  }
}

void PermissionDialogJavaDelegate::ApplyDontAskAgainOption(
    JNIEnv* env,
    PermissionPromptAndroid* permission_prompt) {
  if (permission_prompt->delegate_public()->Requests().size() < 1) {
    return;
  }
  const bool dont_ask_again =
      Java_BravePermissionDialogDelegate_getDontAskAgain(env, j_delegate_);
  const auto& request = permission_prompt->delegate_public()->Requests()[0];
  request->set_dont_ask_again(dont_ask_again);
}

void PermissionDialogJavaDelegate::BravePreCreateDialog(JNIEnv* env) {
#if BUILDFLAG(ENABLE_WIDEVINE)
  if (HasWidevinePermissionRequest(
          permission_prompt_->delegate_public()->Requests())) {
    Java_BravePermissionDialogDelegate_setIsWidevinePermissionRequest(
        env, j_delegate_, true);
  }
#endif
  if (ShouldShowLifetimeOptions(permission_prompt_->delegate_public())) {
    SetLifetimeOptions(j_delegate_);
  }
}

// static
std::unique_ptr<PermissionDialogDelegate> PermissionDialogDelegate::Create(
    content::WebContents* web_contents,
    PermissionPromptAndroid_ChromiumImpl* permission_prompt) {
  // This should never be invoked, created only as a stub.
  // a method with PermissionPromptAndroid* should be invoked instead.
  NOTREACHED();
}

}  // namespace permissions

#define BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT                               \
  java_delegate_->ApplyLifetimeToPermissionRequests(env, permission_prompt_); \
  java_delegate_->ApplyDontAskAgainOption(env, permission_prompt_);
#define BRAVE_PERMISSION_DIALOG_DELEGATE_CANCEL                               \
  java_delegate_->ApplyLifetimeToPermissionRequests(env, permission_prompt_); \
  java_delegate_->ApplyDontAskAgainOption(env, permission_prompt_);
#define BRAVE_PERMISSION_DIALOG_PRE_CREATE_DIALOG BravePreCreateDialog(env);

#include <components/permissions/android/permission_prompt/permission_dialog_delegate.cc>

#undef BRAVE_PERMISSION_DIALOG_PRE_CREATE_DIALOG
#undef BRAVE_PERMISSION_DIALOG_DELEGATE_CANCEL
#undef BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT
