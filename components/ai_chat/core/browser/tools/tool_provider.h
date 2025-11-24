// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_H_

#include <cstdint>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"

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

  // Marks that a new message has been added to the conversation and therefore
  // a new generation loop has started which may result in tool calls.
  // Optionally handle and reset the state of this class or any tools that
  // should only maintain state within the tool loop of a single set of
  // responses. For example a TODO tool would only be applicable during 1 task,
  // but not a whole conversation.
  virtual void OnNewGenerationLoop() {}

  // A response has been completed with no more tool use requests to handle.
  // Future requests might be made in a new loop (after `OnNewGenerationLoop` is
  // called). This is a good opportunity to hand over any control back to the
  // user.
  virtual void OnGenerationCompleteWithNoToolsToHandle() {}

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    // This ToolProvider has some Tool acting on a Tab
    virtual void OnContentTaskStarted(int32_t tab_id) {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

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

  // Attempts to stops all current tasks started by Tools from this
  // ToolProvider.
  virtual void StopAllTasks() {}

 protected:
  base::ObserverList<Observer> observers_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_H_
