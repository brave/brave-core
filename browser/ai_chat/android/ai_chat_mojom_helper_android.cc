/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/android/ai_chat_mojom_helper_android.h"

#include <utility>
#include <vector>

#include "brave/browser/skus/skus_service_factory.h"
#include "brave/build/android/jni_headers/BraveLeoMojomHelper_jni.h"
#include "brave/components/ai_chat/core/browser/models.h"
#include "chrome/browser/browser_process.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/android/browser_context_handle.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

static jlong JNI_BraveLeoMojomHelper_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jbrowser_context_handle) {
  AIChatMojomHelperAndroid* cm_helper =
      new AIChatMojomHelperAndroid(jbrowser_context_handle);
  return reinterpret_cast<intptr_t>(cm_helper);
}

AIChatMojomHelperAndroid::AIChatMojomHelperAndroid(
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

AIChatMojomHelperAndroid::~AIChatMojomHelperAndroid() = default;

void AIChatMojomHelperAndroid::Destroy(JNIEnv* env) {
  delete this;
}

jlong AIChatMojomHelperAndroid::GetInterfaceToAndroidHelper(JNIEnv* env) {
  mojo::PendingRemote<mojom::AIChatAndroidHelper> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());

  return static_cast<jlong>(remote.PassPipe().release().value());
}

void AIChatMojomHelperAndroid::GetPremiumStatus(
    GetPremiumStatusCallback callback) {
  credential_manager_->GetPremiumStatus(
      base::BindOnce(&AIChatMojomHelperAndroid::OnPremiumStatusReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatMojomHelperAndroid::OnPremiumStatusReceived(
    mojom::PageHandler::GetPremiumStatusCallback parent_callback,
    mojom::PremiumStatus premium_status,
    mojom::PremiumInfoPtr premium_info) {
  std::move(parent_callback).Run(premium_status, std::move(premium_info));
}

void AIChatMojomHelperAndroid::GetModelsWithSubtitles(
    GetModelsWithSubtitlesCallback callback) {
  auto all_models = GetAllModels();
  std::vector<mojom::ModelWithSubtitlePtr> models(all_models.size());
  for (size_t i = 0; i < all_models.size(); i++) {
    mojom::ModelWithSubtitle modelWithSubtitle;
    modelWithSubtitle.model = all_models[i].Clone();
    if (modelWithSubtitle.model->key == "chat-basic") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_BASIC_SUBTITLE);
    } else if (modelWithSubtitle.model->key == "chat-leo-expanded") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_LEO_EXPANDED_SUBTITLE);
    } else if (modelWithSubtitle.model->key == "chat-claude-instant") {
      modelWithSubtitle.subtitle =
          l10n_util::GetStringUTF8(IDS_CHAT_UI_CHAT_CLAUDE_INSTANT_SUBTITLE);
    }
    models[i] = modelWithSubtitle.Clone();
  }
  std::move(callback).Run(std::move(models));
}

}  // namespace ai_chat
