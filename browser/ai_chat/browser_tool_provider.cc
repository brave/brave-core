// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_chat/tools/code_execution_tool.h"
#include "brave/browser/brave_tab_helpers.h"
#include "brave/components/ai_chat/content/browser/associated_url_content.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/tools/request_url_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "content/public/browser/browser_context.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL)
#include "brave/browser/ai_chat/tools/tab_management_tool.h"
#endif

namespace ai_chat {

BrowserToolProvider::BrowserToolProvider(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  CreateTools(browser_context);
}

BrowserToolProvider::~BrowserToolProvider() = default;

std::vector<base::WeakPtr<Tool>> BrowserToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tool_ptrs;
  if (code_execution_tool_) {
    tool_ptrs.push_back(code_execution_tool_->GetWeakPtr());
  }

  if (request_url_tool_) {
    tool_ptrs.push_back(request_url_tool_->GetWeakPtr());
  }

#if BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL)
  if (tab_management_tool_) {
    tool_ptrs.push_back(tab_management_tool_->GetWeakPtr());
  }
#endif

  return tool_ptrs;
}

void BrowserToolProvider::OnBoundToConversationHandler(
    ConversationHandler* handler) {
  conversation_handler_ = handler;
}

void BrowserToolProvider::CreateTools(
    content::BrowserContext* browser_context) {
  if (features::IsCodeExecutionToolEnabled()) {
    code_execution_tool_ = std::make_unique<CodeExecutionTool>(browser_context);
  }

  request_url_tool_ = std::make_unique<RequestURLTool>(base::BindRepeating(
      &BrowserToolProvider::AttachURL, weak_ptr_factory_.GetWeakPtr()));

#if BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL)
  if (base::FeatureList::IsEnabled(features::kTabManagementTool)) {
    tab_management_tool_ = std::make_unique<TabManagementTool>();
  }
#endif
}

void BrowserToolProvider::AttachURL(
    GURL url,
    std::string title,
    base::OnceCallback<void(std::string)> on_complete) {
  if (!browser_context_) {
    std::move(on_complete).Run({});
    return;
  }

  auto content = std::make_unique<AssociatedURLContent>(
      url, base::UTF8ToUTF16(title), browser_context_,
      base::BindOnce(&brave::AttachPrivacySensitiveTabHelpers));

  // Call GetContent before moving ownership into the callback so the
  // navigation starts immediately. The unique_ptr is captured as a bound arg
  // to keep the object alive until the page has fully loaded and content
  // extraction is complete, at which point on_complete fires with the text.
  auto* raw_content = content.get();
  raw_content->GetContent(base::BindOnce(
      [](std::unique_ptr<AssociatedURLContent> /* keep_alive */,
         base::OnceCallback<void(std::string)> cb, PageContent page_content) {
        std::move(cb).Run(std::move(page_content.content));
      },
      std::move(content), std::move(on_complete)));
}

}  // namespace ai_chat
