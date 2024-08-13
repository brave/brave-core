/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_TYPES_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_TYPES_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

using SuggestedQuestionResult =
    base::expected<std::vector<std::string>, mojom::APIErrorPtr>;

using SuggestedQuestionsCallback =
    base::OnceCallback<void(SuggestedQuestionResult)>;

using GenerationResult = base::expected<std::string, mojom::APIErrorPtr>;

using GenerationDataCallback =
    base::RepeatingCallback<void(mojom::ConversationEntryEventPtr)>;
using GenerationCompletedCallback = base::OnceCallback<void(GenerationResult)>;

using ConversationHistory = std::vector<mojom::ConversationTurnPtr>;

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_ENGINE_TYPES_H_
