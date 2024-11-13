/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_UTILS_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

class PrefService;
class GURL;

namespace ai_chat {
namespace mojom {
enum class ActionType : int32_t;
}  // namespace mojom

// Check both policy and feature flag to determine if AI Chat is enabled.
bool IsAIChatEnabled(PrefService* prefs);
bool HasUserOptedIn(PrefService* prefs);
void SetUserOptedIn(PrefService* prefs, bool opted_in);

bool IsBraveSearchSERP(const GURL& url);

bool IsPremiumStatus(mojom::PremiumStatus status);

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
using GetOCRTextCallback = base::OnceCallback<void(std::string)>;
void GetOCRText(const SkBitmap& image, GetOCRTextCallback callback);
#endif

const std::string& GetActionTypeQuestion(mojom::ActionType action_type);

EngineConsumer::GenerationDataCallback BindParseRewriteReceivedData(
    ConversationHandler::GeneratedTextCallback callback);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_UTILS_H_
