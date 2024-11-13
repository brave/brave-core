/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/model_validator.h"

#include "base/numerics/safe_math.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/net/base/url_util.h"

class GURL;

namespace ai_chat {

// Static
bool ModelValidator::IsValidContextSize(const std::optional<int32_t>& size) {
  if (!size.has_value()) {
    return false;
  }

  base::CheckedNumeric<size_t> checked_value(size.value());

  return checked_value.IsValid() &&
         checked_value.ValueOrDie() >= kMinCustomModelContextSize &&
         checked_value.ValueOrDie() <= kMaxCustomModelContextSize;
}

// Static
bool ModelValidator::HasValidContextSize(
    const mojom::CustomModelOptions& options) {
  return IsValidContextSize(options.context_size);
}

// Static
bool ModelValidator::IsValidEndpoint(const GURL& endpoint) {
  return net::IsHTTPSOrLocalhostURL(endpoint);
}

ModelValidationResult ModelValidator::ValidateCustomModelOptions(
    const mojom::CustomModelOptions& options) {
  if (!HasValidContextSize(options)) {
    return ModelValidationResult::kInvalidContextSize;
  }

  if (!IsValidEndpoint(options.endpoint)) {
    return ModelValidationResult::kInvalidUrl;
  }

  // Add further validations as needed
  return ModelValidationResult::kSuccess;
}

}  // namespace ai_chat
