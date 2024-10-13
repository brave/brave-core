/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_VALIDATOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_VALIDATOR_H_

#include <optional>

#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom.h"
#include "url/url_constants.h"

namespace ai_chat {

constexpr size_t kMinCustomModelContextSize = 1;
constexpr size_t kMaxCustomModelContextSize = 2'000'000;
constexpr size_t kDefaultCustomModelContextSize = 4000;

enum class ModelValidationResult {
  kSuccess,
  kInvalidContextSize,
  kInvalidUrl,
  // Add other validation results if needed
};

class ModelValidator {
 public:
  // Validates if the context size is within the valid range (1 to 50,000,000)
  static bool IsValidContextSize(const std::optional<int32_t>& context_size);
  static bool HasValidContextSize(const mojom::Model& model);

  static bool IsValidEndpoint(const GURL& endpoint);

  // Validates the custom model's properties, such as context size and endpoint
  static ModelValidationResult ValidateModel(const mojom::Model& model);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_VALIDATOR_H_
