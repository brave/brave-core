/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/android/permission_dialog_delegate.h"

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/android/jni_headers/BravePermissionDialogDelegate_jni.h"
#include "components/permissions/features.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace permissions {
namespace {

const int IDS_PERMISSION_DENY_CHROMIUM_IMPL = IDS_PERMISSION_DENY;
#undef IDS_PERMISSION_DENY
#define IDS_PERMISSION_DENY                                  \
  (ShouldShowLifetimeOptions(permission_prompt_->delegate()) \
       ? IDS_PERMISSIONS_BUBBLE_DENY_FOREVER                 \
       : IDS_PERMISSION_DENY_CHROMIUM_IMPL)

std::vector<PermissionLifetimeOption> CreateAndSetLifetimeOptions(
    const base::android::JavaRef<jobject>& j_delegate) {
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return {};
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BravePermissionDialogDelegate_setLifetimeOptionsText(
      env, j_delegate,
      base::android::ConvertUTF16ToJavaString(
          env, l10n_util::GetStringUTF16(
                   IDS_PERMISSIONS_BUBBLE_LIFETIME_COMBOBOX_LABEL)));

  std::vector<PermissionLifetimeOption> lifetime_options =
      CreatePermissionLifetimeOptions();
  std::vector<base::string16> lifetime_labels;
  for (const auto& lifetime_option : lifetime_options) {
    lifetime_labels.push_back(lifetime_option.label);
  }

  Java_BravePermissionDialogDelegate_setLifetimeOptions(
      env, j_delegate,
      base::android::ToJavaArrayOfStrings(env, lifetime_labels));

  return lifetime_options;
}

}  // namespace
}  // namespace permissions

#define BRAVE_PERMISSION_DIALOG_DELEGATE_CREATE_JAVA_DELEGATE \
  lifetime_options_ = CreateAndSetLifetimeOptions(j_delegate_);
#define BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT \
  ApplyLifetimeToPermissionRequests(env, obj);

#include "../../../../../components/permissions/android/permission_dialog_delegate.cc"

#undef BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT
#undef BRAVE_PERMISSION_DIALOG_DELEGATE_CREATE_JAVA_DELEGATE
#undef IDS_PERMISSION_DENY
#define IDS_PERMISSION_DENY IDS_PERMISSION_DENY_CHROMIUM_IMPL

namespace permissions {

void PermissionDialogDelegate::ApplyLifetimeToPermissionRequests(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return;
  }
  const int selected_lifetime_option =
      Java_BravePermissionDialogDelegate_getSelectedLifetimeOption(env, obj);
  DCHECK(!ShouldShowLifetimeOptions(permission_prompt_->delegate()) ||
         selected_lifetime_option != -1);
  if (selected_lifetime_option != -1) {
    SetRequestsLifetime(lifetime_options_, selected_lifetime_option,
                        permission_prompt_->delegate());
  }
}

}  // namespace permissions
