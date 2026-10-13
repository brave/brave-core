// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MEMORY_STORAGE_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MEMORY_STORAGE_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

class PrefService;

namespace ai_chat {

// Tool for storing user memories locally in prefs, which is owned by
// AIChatService and shared across conversations.
//
// Assistants are supposed to use this tool only when the user explicitly asks
// to remember something. A fact that the user only mentions is left to the
// learning from the saved chats. Memory passed in the tool_use event will be
// added into our memory preference automatically in UseTool. The tool adds
// an artifact that tells if the memory is new or was saved already. UI is
// responsible to present the memory to user for visibility, and an option to
// undo the operation when the tool added it.
class MemoryStorageTool : public Tool {
 public:
  // Called after each save with the memory, and whether the tool added it
  // (true) or the list had it already (false).
  using SaveCallback =
      base::RepeatingCallback<void(const std::string& memory, bool added)>;

  // |on_save| can be null.
  explicit MemoryStorageTool(PrefService* pref_service,
                             SaveCallback on_save = SaveCallback());
  ~MemoryStorageTool() override;

  MemoryStorageTool(const MemoryStorageTool&) = delete;
  MemoryStorageTool& operator=(const MemoryStorageTool&) = delete;

  // Tool overrides
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool SupportsConversation(bool is_temporary,
                            bool has_untrusted_content,
                            const ConversationCapabilitySet&
                                conversation_capabilities) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  raw_ptr<PrefService> pref_service_;
  SaveCallback on_save_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MEMORY_STORAGE_TOOL_H_
