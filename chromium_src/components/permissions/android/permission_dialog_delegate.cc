/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/android/permission_dialog_delegate.h"

#include "brave/components/permissions/permission_lifetime_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/android/jni_headers/BravePermissionDialogDelegate_jni.h"
#include "components/permissions/features.h"
#include "components/strings/grit/components_strings.h"

namespace {

const int IDS_PERMISSION_DENY_CHROMIUM_IMPL = IDS_PERMISSION_DENY;
#undef IDS_PERMISSION_DENY
#define IDS_PERMISSION_DENY                                  \
  (ShouldShowLifetimeOptions(permission_prompt_->delegate()) \
       ? IDS_PERMISSIONS_BUBBLE_DENY_FOREVER                 \
       : IDS_PERMISSION_DENY_CHROMIUM_IMPL)

}  // namespace

#define BRAVE_PERMISSION_DIALOG_DELEGATE_CREATE_JAVA_DELEGATE \
  CreateAndSetLifetimeOptions();
#define BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT \
  ApplyLifetimeToPermissionRequests(env, obj);

#include "../../../../../../components/permissions/android/permission_dialog_delegate.cc"

#undef BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT
#undef BRAVE_PERMISSION_DIALOG_DELEGATE_CREATE_JAVA_DELEGATE
#undef IDS_PERMISSION_DENY
#define IDS_PERMISSION_DENY IDS_PERMISSION_DENY_CHROMIUM_IMPL

namespace permissions {

void PermissionDialogDelegate::CreateAndSetLifetimeOptions() {
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return;
  }
  lifetime_options_ = CreatePermissionLifetimeOptions();
  std::vector<base::string16> lifetime_labels;
  for (const auto& lifetime_option : lifetime_options_) {
    lifetime_labels.push_back(lifetime_option.label);
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BravePermissionDialogDelegate_setLifetimeOptions(
      env, j_delegate_,
      base::android::ToJavaArrayOfStrings(env, lifetime_labels));
}

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
