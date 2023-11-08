// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODELS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODELS_H_

#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

extern const char kModelsDefaultKey[];
extern const char kModelsPremiumDefaultKey[];

// All models that the user can choose for chat conversations.
extern const base::flat_map<std::string_view, mojom::Model> kAllModels;
// UI display order for models
extern const std::vector<std::string_view> kAllModelKeysDisplayOrder;

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODELS_H_
