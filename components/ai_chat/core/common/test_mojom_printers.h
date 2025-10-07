// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_MOJOM_PRINTERS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_MOJOM_PRINTERS_H_

#include <string>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

namespace mojom {

// Define the PrintTo function for a mojo type and also the StructPtr version
// to allow for passing either type during tests.
#define GENERATE_MOJO_PTR_PRINTER(Type)                         \
  void PrintTo(const Type& instance, std::ostream* os);         \
  inline void PrintTo(const Type##Ptr& ptr, std::ostream* os) { \
    PrintTo(*ptr, os);                                          \
  }

GENERATE_MOJO_PTR_PRINTER(AssociatedContent)
GENERATE_MOJO_PTR_PRINTER(Conversation)
GENERATE_MOJO_PTR_PRINTER(ToolUseEvent)
GENERATE_MOJO_PTR_PRINTER(ConversationEntryEvent)
GENERATE_MOJO_PTR_PRINTER(ConversationTurn)

}  // namespace mojom

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_TEST_MOJOM_PRINTERS_H_
