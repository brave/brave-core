/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/model_validator.h"

#include <limits>
#include <optional>

#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/settings_helper.mojom.h"
#include "brave/net/base/url_util.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

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
bool ModelValidator::IsValidEndpoint(const GURL& endpoint) {
  return net::IsHTTPSOrLocalhostURL(endpoint);
}

ModelValidationResult ModelValidator::ValidateModel(const mojom::Model& model) {
  if (!IsValidContextSize(
          model.options->get_custom_model_options()->context_size)) {
    return ModelValidationResult::kInvalidContextSize;
  }

  if (!IsValidEndpoint(model.options->get_custom_model_options()->endpoint)) {
    return ModelValidationResult::kInvalidUrl;
  }

  // Add further validations as needed
  return ModelValidationResult::kSuccess;
}

}  // namespace ai_chat
