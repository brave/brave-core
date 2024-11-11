/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_VALIDATOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_VALIDATOR_H_

#include <optional>

#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/net/base/url_util.h"
#include "url/url_constants.h"

namespace ai_chat {

// The declared context size needs to be large enough to accommodate expected
// reserves (i.e., prompt tokens and max new tokens)
inline constexpr size_t kMinCustomModelContextSize =
    kReservedTokensForMaxNewTokens + kReservedTokensForPrompt;
inline constexpr size_t kMaxCustomModelContextSize = 2'000'000;
inline constexpr size_t kDefaultCustomModelContextSize = 4000;

enum class ModelValidationResult {
  kSuccess,
  kInvalidContextSize,
  kInvalidUrl,
  // Add other validation results if needed
};

class ModelValidator {
 public:
  // Validates if the context size is within the valid range (1 to 50,000,000)
  static bool IsValidContextSize(const std::optional<int32_t>& size);
  static bool HasValidContextSize(const mojom::CustomModelOptions& options);

  static bool IsValidEndpoint(const GURL& endpoint);

  // Validates the custom model's properties, such as context size and endpoint
  static ModelValidationResult ValidateCustomModelOptions(
      const mojom::CustomModelOptions& options);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MODEL_VALIDATOR_H_
