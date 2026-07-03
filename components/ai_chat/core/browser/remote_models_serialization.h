// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_SERIALIZATION_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_SERIALIZATION_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace ai_chat {

// JSON field names shared between the server response, the disk cache, and the
// serializer. Both ParseModelsFromJSON and SerializeModels must use the same
// keys so that serialized models can be re-parsed without modification.
inline constexpr char kModelsKey[] = "models";
inline constexpr char kKeyField[] = "key";
inline constexpr char kDisplayNameField[] = "display_name";
inline constexpr char kCapabilitiesField[] = "capabilities";
inline constexpr char kIsSuggestedModelField[] = "is_suggested_model";
inline constexpr char kIsNearModelField[] = "is_near_model";
inline constexpr char kOptionsField[] = "options";
inline constexpr char kTypeField[] = "type";
inline constexpr char kLeoType[] = "leo";
inline constexpr char kNameField[] = "name";
inline constexpr char kDisplayMakerField[] = "display_maker";
inline constexpr char kDescriptionField[] = "description";
inline constexpr char kAccessField[] = "access";
inline constexpr char kMaxAssociatedContentLengthField[] =
    "max_associated_content_length";
inline constexpr char kLongConversationWarningCharacterLimitField[] =
    "long_conversation_warning_character_limit";

// Parses a JSON value (either a dict with a "models" key, or a bare array)
// and returns the successfully parsed models. Entries that fail field
// validation are silently skipped.
std::vector<mojom::ModelPtr> ParseModelsFromJSON(const base::Value& json);

// Serializes |models| to a JSON list value using the server JSON format, so
// that ParseModelsFromJSON can re-parse the result without modification.
base::ListValue SerializeModels(const std::vector<mojom::ModelPtr>& models);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_REMOTE_MODELS_SERIALIZATION_H_
