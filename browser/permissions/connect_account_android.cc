/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/ConnectAccountFragment_jni.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "content/public/browser/web_contents.h"

#include "base/functional/callback.h"
#include "content/public/browser/render_frame_host.h"

namespace {

base::android::ScopedJavaLocalRef<jobject> GetJavaBoolean(JNIEnv* env,
                                                          bool native_bool) {
  jclass booleanClass = env->FindClass("java/lang/Boolean");
  jmethodID methodID = env->GetMethodID(booleanClass, "<init>", "(Z)V");
  jobject booleanObject = env->NewObject(booleanClass, methodID, native_bool);

  return base::android::ScopedJavaLocalRef<jobject>(env, booleanObject);
}

void PlainCallConnectAccountCallback(
    JNIEnv* env,
    base::android::ScopedJavaGlobalRef<jobject> java_callback,
    bool value_result) {
  Java_ConnectAccountFragment_onConnectAccountDone(
      env, java_callback, GetJavaBoolean(env, value_result));
}

}  // namespace

static void JNI_ConnectAccountFragment_ConnectAccount(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& java_account_address,
    jint account_id_coin,
    const base::android::JavaParamRef<jobject>& java_web_contents,
    const base::android::JavaParamRef<jobject>& callback) {
  base::android::ScopedJavaGlobalRef<jobject> java_callback;
  java_callback.Reset(env, callback);

  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(java_web_contents);

  if (web_contents == nullptr) {
    PlainCallConnectAccountCallback(env, java_callback, false);
    return;
  }

  std::string account_address =
      base::android::ConvertJavaStringToUTF8(java_account_address);

  content::RenderFrameHost* rfh = web_contents->GetFocusedFrame();
  if (rfh == nullptr) {
    PlainCallConnectAccountCallback(env, java_callback, false);
    return;
  }

  brave_wallet::mojom::CoinType coin =
      static_cast<brave_wallet::mojom::CoinType>(account_id_coin);
  CHECK(brave_wallet::mojom::IsKnownEnumValue(coin));

  auto request_type = brave_wallet::CoinTypeToPermissionRequestType(coin);
  auto permission = brave_wallet::CoinTypeToPermissionType(coin);

  if (!request_type || !permission) {
    PlainCallConnectAccountCallback(env, java_callback, false);
    return;
  }

  if (permissions::BraveWalletPermissionContext::HasRequestsInProgress(
          rfh, *request_type)) {
    PlainCallConnectAccountCallback(env, java_callback, false);
    return;
  }

  permissions::BraveWalletPermissionContext::RequestPermissions(
      *permission, rfh, {account_address},
      base::BindOnce(
          [](JNIEnv* env,
             base::android::ScopedJavaGlobalRef<jobject> java_callback,
             const std::vector<blink::mojom::PermissionStatus>& responses) {
            if (responses.empty() || responses.size() != 1u) {
              PlainCallConnectAccountCallback(env, java_callback, false);
            } else {
              PlainCallConnectAccountCallback(env, java_callback, true);
            }
          },
          env, std::move(java_callback)));
}
