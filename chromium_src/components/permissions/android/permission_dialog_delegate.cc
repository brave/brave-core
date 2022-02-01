/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/android/permission_dialog_delegate.h"

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/permissions/android/jni_headers/BravePermissionDialogDelegate_jni.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/android/jni_headers/PermissionDialogController_jni.h"
#include "components/permissions/android/permission_prompt_android.h"
#include "components/permissions/features.h"
#include "components/permissions/permission_request.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace permissions {
namespace {

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

void ApplyLifetimeToPermissionRequests(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    PermissionPromptAndroid* permission_prompt) {
  std::vector<PermissionRequest*> requests =
      permission_prompt->delegate()->Requests();
  if (requests.size() != 0 &&
      requests[0]->request_type() == permissions::RequestType::kBraveEthereum) {
    // TODO(sergz) make a proper clean up as we don't call it on dialog dismiss
    // currently
    Java_BravePermissionDialogDelegate_disconnectMojoServices(env, obj);

    return;
  }
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return;
  }
  const int selected_lifetime_option =
      Java_BravePermissionDialogDelegate_getSelectedLifetimeOption(env, obj);
  DCHECK(!ShouldShowLifetimeOptions(permission_prompt->delegate()) ||
         selected_lifetime_option != -1);
  if (selected_lifetime_option != -1) {
    std::vector<PermissionLifetimeOption> lifetime_options =
        CreatePermissionLifetimeOptions();
    SetRequestsLifetime(lifetime_options, selected_lifetime_option,
                        permission_prompt->delegate());
  }
}

void AddWalletParamsFromPermissionRequests(
    JNIEnv* env,
    base::android::ScopedJavaGlobalRef<jobject> j_delegate,
    PermissionPromptAndroid* permission_prompt) {
  std::vector<PermissionRequest*> requests =
      permission_prompt->delegate()->Requests();
  if (requests.size() == 0 ||
      requests[0]->request_type() != permissions::RequestType::kBraveEthereum) {
    return;
  }
  Java_BravePermissionDialogDelegate_setUseWalletLayout(env, j_delegate, true);
}

void Java_PermissionDialogController_createDialog_BraveImpl(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& delegate) {
  SetLifetimeOptions(delegate);
  Java_PermissionDialogController_createDialog(env, delegate);
}

}  // namespace
}  // namespace permissions

#define BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT                                \
  ApplyLifetimeToPermissionRequests(env, obj, permission_prompt_);             \
  std::vector<PermissionRequest*> requests =                                   \
      permission_prompt_->delegate()->Requests();                              \
  if (requests.size() != 0 &&                                                  \
      requests[0]->request_type() == permissions::RequestType::kBraveEthereum) \
    return;
#define BRAVE_PERMISSION_DIALOG_DELEGATE_CANCEL                                \
  ApplyLifetimeToPermissionRequests(env, obj, permission_prompt_);             \
  std::vector<PermissionRequest*> requests =                                   \
      permission_prompt_->delegate()->Requests();                              \
  if (requests.size() != 0 &&                                                  \
      requests[0]->request_type() == permissions::RequestType::kBraveEthereum) \
    return;
#define BRAVE_PERMISSION_DIALOG_DELEGATE_ADD_WALLET_PARAMS \
  AddWalletParamsFromPermissionRequests(env, j_delegate_, permission_prompt_);
#define Java_PermissionDialogController_createDialog \
  Java_PermissionDialogController_createDialog_BraveImpl

#include "src/components/permissions/android/permission_dialog_delegate.cc"

#undef Java_PermissionDialogController_createDialog
#undef BRAVE_PERMISSION_DIALOG_DELEGATE_CANCEL
#undef BRAVE_PERMISSION_DIALOG_DELEGATE_ACCEPT
