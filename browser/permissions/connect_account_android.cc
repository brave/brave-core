/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/ConnectAccountFragment_jni.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "content/public/browser/web_contents.h"

static void JNI_ConnectAccountFragment_ConnectAccount(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& java_account_address,
    const base::android::JavaParamRef<jobject>& java_web_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(java_web_contents);
  if (web_contents == nullptr) {
    return;
  }

  int permission_lifetime_option =
      static_cast<int>(brave_wallet::mojom::PermissionLifetimeOption::kForever);
  std::string account_address =
      base::android::ConvertJavaStringToUTF8(java_account_address);

  brave_wallet::mojom::PermissionLifetimeOption lifetime_enum =
      static_cast<brave_wallet::mojom::PermissionLifetimeOption>(
          permission_lifetime_option);
  CHECK(IsKnownEnumValue(lifetime_enum));
  permissions::BraveWalletPermissionContext::AcceptOrCancel(
      {account_address}, lifetime_enum, web_contents);
}
