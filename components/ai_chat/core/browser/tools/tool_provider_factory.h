// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_FACTORY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_FACTORY_H_

#include <memory>

namespace ai_chat {

class ToolProvider;

// Factory interface for creating ToolProvider instances.
// Since each ToolProvider is owned by the caller and may have state stored
// by conversation and even between-tools, this factory exists to construct an
// instance of the ToolProvider for new conversations or wherever tool calls are
// neccessary.
class ToolProviderFactory {
 public:
  virtual ~ToolProviderFactory() = default;

  // Creates a new ToolProvider instance for a conversation
  virtual std::unique_ptr<ToolProvider> CreateToolProvider() = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_PROVIDER_FACTORY_H_
