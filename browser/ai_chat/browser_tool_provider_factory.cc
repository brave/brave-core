// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider_factory.h"

#include <memory>

#include "brave/browser/ai_chat/browser_tool_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {

BrowserToolProviderFactory::BrowserToolProviderFactory(Profile* profile)
    : profile_(profile) {}

BrowserToolProviderFactory::~BrowserToolProviderFactory() = default;

std::unique_ptr<ToolProvider> BrowserToolProviderFactory::CreateToolProvider() {
  return std::make_unique<BrowserToolProvider>(profile_);
}

}  // namespace ai_chat
