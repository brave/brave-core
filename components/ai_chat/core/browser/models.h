// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODELS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODELS_H_

#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

// All models that the user can choose for chat conversations, in UI display
// order.
extern const std::vector<ai_chat::mojom::Model>& GetAllModels();

// Get model by key. If there is no matching model for the key, NULL is
// returned.
extern const ai_chat::mojom::Model* GetModel(std::string_view key);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODELS_H_
