// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_FACTORY_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider_factory.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

// Factory for creating ToolProvider instances in the browser layer
class BrowserToolProviderFactory : public ToolProviderFactory {
 public:
  explicit BrowserToolProviderFactory(content::BrowserContext* browser_context);
  ~BrowserToolProviderFactory() override;

  BrowserToolProviderFactory(const BrowserToolProviderFactory&) = delete;
  BrowserToolProviderFactory& operator=(const BrowserToolProviderFactory&) =
      delete;

  // ToolProviderFactory implementation
  std::unique_ptr<ToolProvider> CreateToolProvider() override;

 private:
  raw_ptr<content::BrowserContext> browser_context_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_BROWSER_TOOL_PROVIDER_FACTORY_H_
