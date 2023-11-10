/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/android/ai_chat_cm_helper_android.h"

#include <utility>

#include "brave/browser/skus/skus_service_factory.h"
#include "brave/build/android/jni_headers/BraveLeoCMHelper_jni.h"
#include "chrome/browser/browser_process.h"
#include "content/public/browser/android/browser_context_handle.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace ai_chat {

static jlong JNI_BraveLeoCMHelper_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jbrowser_context_handle) {
  AIChatCMHelperAndroid* cm_helper =
      new AIChatCMHelperAndroid(jbrowser_context_handle);
  return reinterpret_cast<intptr_t>(cm_helper);
}

AIChatCMHelperAndroid::AIChatCMHelperAndroid(
    const base::android::JavaParamRef<jobject>& jbrowser_context_handle) {
  content::BrowserContext* context =
      content::BrowserContextFromJavaHandle(jbrowser_context_handle);
  auto skus_service_getter = base::BindRepeating(
      [](content::BrowserContext* context) {
        return skus::SkusServiceFactory::GetForContext(context);
      },
      context);
  credential_manager_ = std::make_unique<ai_chat::AIChatCredentialManager>(
      skus_service_getter, g_browser_process->local_state());
}

AIChatCMHelperAndroid::~AIChatCMHelperAndroid() = default;

void AIChatCMHelperAndroid::Destroy(JNIEnv* env) {
  delete this;
}

jlong AIChatCMHelperAndroid::GetInterfaceToCredentialManagerHelper(
    JNIEnv* env) {
  mojo::PendingRemote<mojom::CredentialManagerHelper> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());

  return static_cast<jlong>(remote.PassPipe().release().value());
}

void AIChatCMHelperAndroid::GetPremiumStatus(
    GetPremiumStatusCallback callback) {
  credential_manager_->GetPremiumStatus(
      base::BindOnce(&AIChatCMHelperAndroid::OnPremiumStatusReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatCMHelperAndroid::OnPremiumStatusReceived(
    mojom::PageHandler::GetPremiumStatusCallback parent_callback,
    mojom::PremiumStatus premium_status) {
  std::move(parent_callback).Run(premium_status);
}

}  // namespace ai_chat
