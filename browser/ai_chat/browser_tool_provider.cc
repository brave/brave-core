// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider.h"

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

BrowserToolProvider::BrowserToolProvider() {
  CreateTools();
}

BrowserToolProvider::~BrowserToolProvider() = default;

std::vector<base::WeakPtr<Tool>> BrowserToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tool_ptrs;
  // TODO(petemill): Return some tools
  return tool_ptrs;
}

void BrowserToolProvider::CreateTools() {
  // TODO(petemill): Construct some tools and own them
}

}  // namespace ai_chat
