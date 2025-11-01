// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider.h"

#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(ENABLE_TAB_MANAGEMENT_TOOL)
#include "brave/browser/ai_chat/tools/tab_management_tool.h"
#endif

namespace ai_chat {

BrowserToolProvider::BrowserToolProvider(Profile* profile) : profile_(profile) {
  CreateTools();
}

BrowserToolProvider::~BrowserToolProvider() = default;

std::vector<base::WeakPtr<Tool>> BrowserToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tool_ptrs;

#if BUILDFLAG(ENABLE_TAB_MANAGEMENT_TOOL)
  tool_ptrs.push_back(tab_management_tool_->GetWeakPtr());
#endif

  return tool_ptrs;
}

void BrowserToolProvider::CreateTools() {
#if BUILDFLAG(ENABLE_TAB_MANAGEMENT_TOOL)
  tab_management_tool_ = std::make_unique<TabManagementTool>(profile_);
#endif
}

}  // namespace ai_chat
