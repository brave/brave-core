// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/memory_storage_tool.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
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

MemoryStorageTool::MemoryStorageTool(PrefService* pref_service,
                                     SaveCallback on_save)
    : pref_service_(pref_service), on_save_(std::move(on_save)) {
  CHECK(pref_service);
}

MemoryStorageTool::~MemoryStorageTool() = default;

std::string_view MemoryStorageTool::Name() const {
  return mojom::kMemoryStorageToolName;
}

std::string_view MemoryStorageTool::Description() const {
  return "Store user information ONLY when the user explicitly asks you to "
         "remember it, with phrases like 'Remember that I...', 'Please note "
         "that I...', 'Store that I...', 'Save this to memory' or similar "
         "direct memory commands. Do NOT use it when the user only mentions "
         "something about themselves, for example 'I moved to Italy', 'I am a "
         "teacher' or 'I like tea': a later step learns such facts without "
         "this tool. Do NOT use it for examples, questions, or context. If "
         "you are not sure that the user asked you to remember it, do not "
         "call this tool. Returns empty string on success, error message on "
         "failure. Call this tool at most once per turn.";
}

std::optional<base::DictValue> MemoryStorageTool::InputProperties() const {
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

bool MemoryStorageTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    const ConversationCapabilitySet& conversation_capabilities) const {
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
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Error: Invalid JSON input, input must be a JSON object"),
        {});
    return;
  }

  const std::string* memory_content = input_dict->FindString("memory");

  if (!memory_content || memory_content->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'memory' field"),
        {});
    return;
  }

  // Validate memory length
  if (memory_content->length() > mojom::kMaxMemoryRecordLength) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            base::StrCat({"Error: Memory content exceeds ",
                          base::NumberToString(mojom::kMaxMemoryRecordLength),
                          " character limit"})),
        {});
    return;
  }

  // Store the memory using the prefs utility
  const bool added = prefs::AddMemoryToPrefs(*memory_content, *pref_service_);
  if (on_save_) {
    on_save_.Run(*memory_content, added);
  }

  // Return empty result to signal the completion of this tool to AI agents.
  // The artifact tells the UI if the memory is new.
  std::vector<mojom::ToolArtifactPtr> artifacts;
  artifacts.push_back(mojom::ToolArtifact::New(
      /*id=*/std::nullopt, mojom::kMemorySaveStatusArtifactType,
      base::WriteJson(
          base::DictValue().Set("status", added ? "added" : "duplicate"))
          .value_or("{}")));
  std::move(callback).Run(CreateContentBlocksForText(""), std::move(artifacts));
}

}  // namespace ai_chat
