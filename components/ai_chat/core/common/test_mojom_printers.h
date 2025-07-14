// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_MOJOM_PRINTERS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_MOJOM_PRINTERS_H_

#include <string>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

namespace mojom {

#define GENERATE_MOJO_PTR_PRINTER(Type)                         \
  inline void PrintTo(const Type##Ptr& ptr, std::ostream* os) { \
    PrintTo(*ptr, os);                                          \
  }

void PrintTo(const AssociatedContent& content, std::ostream* os);
void PrintTo(const Conversation& conversation, std::ostream* os);
void PrintTo(const ToolUseEvent& event, std::ostream* os);
void PrintTo(const ConversationEntryEvent& event, std::ostream* os);
void PrintTo(const ConversationTurn& event, std::ostream* os);

GENERATE_MOJO_PTR_PRINTER(AssociatedContent)
GENERATE_MOJO_PTR_PRINTER(Conversation)
GENERATE_MOJO_PTR_PRINTER(ToolUseEvent)
GENERATE_MOJO_PTR_PRINTER(ConversationEntryEvent)
GENERATE_MOJO_PTR_PRINTER(ConversationTurn)

}  // namespace mojom

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_MOJOM_PRINTERS_H_
