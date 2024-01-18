/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/android/jni_headers/BraveLeoUtils_jni.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "content/public/browser/web_contents.h"
#endif

namespace ai_chat {

static void JNI_BraveLeoUtils_OpenLeoQuery(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents,
    const base::android::JavaParamRef<jstring>& query) {
#if BUILDFLAG(ENABLE_AI_CHAT)
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  // Send the query to the AIChat's backend.
  auto* chat_tab_helper =
      ai_chat::AIChatTabHelper::FromWebContents(web_contents);
  DCHECK(chat_tab_helper);
  ai_chat::mojom::ConversationTurn turn = {
      ai_chat::mojom::CharacterType::HUMAN,
      ai_chat::mojom::ConversationTurnVisibility::VISIBLE,
      base::android::ConvertJavaStringToUTF8(query)};
  chat_tab_helper->SubmitHumanConversationEntry(std::move(turn));
#endif
}
}  // namespace ai_chat
