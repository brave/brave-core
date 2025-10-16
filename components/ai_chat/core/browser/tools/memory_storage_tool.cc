// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/memory_storage_tool.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

MemoryStorageTool::MemoryStorageTool(PrefService* pref_service)
    : pref_service_(pref_service) {
  CHECK(pref_service);
}

MemoryStorageTool::~MemoryStorageTool() = default;

std::string_view MemoryStorageTool::Name() const {
  return mojom::kMemoryStorageToolName;
}

std::string_view MemoryStorageTool::Description() const {
  return "Store user information ONLY when user explicitly requests to "
         "remember something "
         "with phrases like 'Remember that I...', 'Please note that I...', "
         "'Store that I...', "
         "or similar direct memory commands. Do NOT use for casual mentions, "
         "examples, "
         "questions, or context. Returns empty string on success, error "
         "message "
         "on failure. Call this tool at most once per turn.";
}

std::optional<base::Value::Dict> MemoryStorageTool::InputProperties() const {
  std::string description = base::StrCat(
      {"Store ONLY the new information the user just asked to be "
       "remembered. "
       "Maximum ",
       base::NumberToString(mojom::kMaxMemoryRecordLength),
       " characters. "
       "Write in simple, direct statements without explanations or "
       "meta-commentary. "
       "Do NOT include information already in <user_memory> context."
       "Do NOT start with 'User' or add explanations about why this helps "
       "future conversations. "
       "Examples: "
       "'Python developer working on machine learning projects' "
       "'Prefers concise explanations without verbose introductions' "
       "'Lives in Pacific timezone, works 9-5 Monday-Friday' "
       "'Learning React, prefers TypeScript examples' "
       "'Likes cats'"});

  return CreateInputProperties({{"memory", StringProperty(description)}});
}

std::optional<std::vector<std::string>> MemoryStorageTool::RequiredProperties()
    const {
  return std::vector<std::string>{"memory"};
}

bool MemoryStorageTool::RequiresUserInteractionBeforeHandling() const {
  return false;
}

bool MemoryStorageTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    mojom::ConversationCapability conversation_capability) const {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  return false;
#else
  return !is_temporary && !has_untrusted_content;
#endif
}

void MemoryStorageTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  // Parse the input JSON
  auto input_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Invalid JSON input, input must be a JSON object"));
    return;
  }

  const std::string* memory_content = input_dict->FindString("memory");

  if (!memory_content || memory_content->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'memory' field"));
    return;
  }

  // Validate memory length
  if (memory_content->length() > mojom::kMaxMemoryRecordLength) {
    std::move(callback).Run(CreateContentBlocksForText(
        base::StrCat({"Error: Memory content exceeds ",
                      base::NumberToString(mojom::kMaxMemoryRecordLength),
                      " character limit"})));
    return;
  }

  // Store the memory using the prefs utility
  prefs::AddMemoryToPrefs(*memory_content, *pref_service_);

  // Return empty result to signal the completion of this tool to AI agents.
  std::move(callback).Run(CreateContentBlocksForText(""));
}

}  // namespace ai_chat
