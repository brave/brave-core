// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_H_

#include <vector>

#include "base/memory/weak_ptr.h"

namespace ai_chat {

class Tool;

// Interface for providing tools to a conversation.
// The purpose is to:
// 1) Allow different layers to provide tools to all conversations
// 2) Allow tools to be grouped by concerns, and share state between them. For
//    example a set of tools that gets, sets or modifies a shared piece of data.
// 3) Allow knowledge of a Tool, it's creation, filtering and data to be
// concentrated in a single place, and not inside ConversationHandler or
// AIChatService. For simplicity we keep the lifecycle the same for all
// ToolProviders: 1 instance of each ToolProvider per ConversationHandler. Each
// ConversationHandler owns 1 instance of each ToolProvider so that any state
// is attached to a single conversation.
// If you need to share state between conversations, you can use an external
// data store, such as prefs, something in-memory, or a ToolProviderFactory that
// can provide a reference to that data via constructor parameters.
class ToolProvider {
 public:
  ToolProvider();
  virtual ~ToolProvider();

  ToolProvider(const ToolProvider&) = delete;
  ToolProvider& operator=(const ToolProvider&) = delete;

  // Optionally handle and reset the state of this class or any tools that
  // should only maintain state within the tool loop of a single set of
  // responses. For example a TODO tool would only be applicable during 1 task,
  // but not a whole conversation.
  virtual void OnNewGenerationLoop() {}

  // Returns the list of tools available for the conversation.
  // The returned pointers *should* be valid as long as the ToolProvider exists
  // until either the ToolProvider is destroyed, or `OnNewGenerationLoop` is
  // called. Implementors should aim to not destroy any tools outside of
  // `OnNewGenerationLoop`, so that Tools don't go away mid-loop and leave
  // conversations hanging waiting for a response or not finding a tool that's
  // been requested.
  // Note: any filtering conditions required by ToolProviders can be added as
  // params here.
  virtual std::vector<base::WeakPtr<Tool>> GetTools() = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_H_
