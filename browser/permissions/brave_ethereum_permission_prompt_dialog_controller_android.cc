/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/permissions/brave_ethereum_permission_prompt_dialog_controller_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/BraveEthereumPermissionPromptDialog_jni.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/mojom/favicon/favicon_url.mojom.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "url/gurl.h"

namespace {

GURL GetFavIconURL(const std::vector<blink::mojom::FaviconURLPtr>& candidates) {
  for (const auto& candidate : candidates) {
    if (!candidate->icon_url.is_valid() ||
        candidate->icon_type != blink::mojom::FaviconIconType::kFavicon)
      continue;

    return candidate->icon_url;
  }

  return GURL();
}

}  // namespace

BraveEthereumPermissionPromptDialogController::
    BraveEthereumPermissionPromptDialogController(
        Delegate* delegate,
        content::WebContents* web_contents)
    : delegate_(delegate), web_contents_(web_contents) {}

BraveEthereumPermissionPromptDialogController::
    ~BraveEthereumPermissionPromptDialogController() {
  DismissDialog();
}

void BraveEthereumPermissionPromptDialogController::ShowDialog() {
  if (!GetOrCreateJavaObject())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveEthereumPermissionPromptDialog_show(env, GetOrCreateJavaObject());
}

void BraveEthereumPermissionPromptDialogController::OnPrimaryButtonClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobjectArray>& accounts) {
  std::vector<std::string> allowedAccounts;
  base::android::AppendJavaStringArrayToStringVector(env, accounts,
                                                     &allowedAccounts);
  delegate_->ConnectToSite(allowedAccounts);
}

void BraveEthereumPermissionPromptDialogController::OnNegativeButtonClicked(
    JNIEnv* env) {
  delegate_->CancelConnectToSite();
}

void BraveEthereumPermissionPromptDialogController::OnDialogDismissed(
    JNIEnv* env) {
  java_object_.Reset();
  delegate_->OnDialogDismissed();
}

void BraveEthereumPermissionPromptDialogController::DismissDialog() {
  if (java_object_) {
    Java_BraveEthereumPermissionPromptDialog_dismissDialog(
        base::android::AttachCurrentThread(), java_object_);
  }
}

base::android::ScopedJavaGlobalRef<jobject>
BraveEthereumPermissionPromptDialogController::GetOrCreateJavaObject() {
  if (java_object_)
    return java_object_;

  if (web_contents_->GetNativeView() == nullptr ||
      web_contents_->GetNativeView()->GetWindowAndroid() == nullptr)
    return nullptr;  // No window attached (yet or anymore).

  GURL fav_icon_url = GetFavIconURL(web_contents_->GetFaviconURLs());
  JNIEnv* env = base::android::AttachCurrentThread();
  ui::ViewAndroid* view_android = web_contents_->GetNativeView();
  return java_object_ = Java_BraveEthereumPermissionPromptDialog_create(
             env, reinterpret_cast<intptr_t>(this),
             view_android->GetWindowAndroid()->GetJavaObject(),
             web_contents_->GetJavaWebContents(),
             base::android::ConvertUTF8ToJavaString(
                 env, fav_icon_url.is_valid() ? fav_icon_url.spec() : ""));
}
