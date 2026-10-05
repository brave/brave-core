// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/browser_tool_provider.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_chat/tools/code_execution_tool.h"
#include "brave/browser/ai_chat/tools/history_search_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL)
#include "brave/browser/ai_chat/tools/tab_management_tool.h"
#endif

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/browser/ai_chat/ai_chat_embeddings_service_factory.h"
#include "brave/browser/ai_chat/tools/conversation_search_tool.h"
#include "brave/browser/ai_chat/tools/memory_semantic_search_tool.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#endif

namespace ai_chat {

BrowserToolProvider::BrowserToolProvider(Profile* profile,
                                         std::string conversation_uuid)
    : conversation_uuid_(std::move(conversation_uuid)), profile_(profile) {
  CreateTools(profile);
}

BrowserToolProvider::~BrowserToolProvider() = default;

std::vector<base::WeakPtr<Tool>> BrowserToolProvider::GetTools() {
  std::vector<base::WeakPtr<Tool>> tool_ptrs;
  if (code_execution_tool_) {
    tool_ptrs.push_back(code_execution_tool_->GetWeakPtr());
  }
  if (history_search_tool_) {
    tool_ptrs.push_back(history_search_tool_->GetWeakPtr());
  }
#if BUILDFLAG(ENABLE_LOCAL_AI)
  if (conversation_search_tool_) {
    tool_ptrs.push_back(conversation_search_tool_->GetWeakPtr());
  }
  if (memory_semantic_search_tool_) {
    tool_ptrs.push_back(memory_semantic_search_tool_->GetWeakPtr());
  }
#endif

#if BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL)
  if (tab_management_tool_) {
    tool_ptrs.push_back(tab_management_tool_->GetWeakPtr());
  }
#endif

  return tool_ptrs;
}

HistorySearchTool* BrowserToolProvider::GetHistorySearchToolForTesting() {
  CHECK_IS_TEST();
  return history_search_tool_.get();
}

#if BUILDFLAG(ENABLE_LOCAL_AI)
ConversationSearchTool*
BrowserToolProvider::GetConversationSearchToolForTesting() {
  CHECK_IS_TEST();
  return conversation_search_tool_.get();
}

MemorySemanticSearchTool*
BrowserToolProvider::GetMemorySemanticSearchToolForTesting() {
  CHECK_IS_TEST();
  return memory_semantic_search_tool_.get();
}
#endif

void BrowserToolProvider::CreateTools(
    content::BrowserContext* browser_context) {
  if (features::IsCodeExecutionToolEnabled()) {
    code_execution_tool_ = std::make_unique<CodeExecutionTool>(browser_context);
  }
  if (history_embeddings::IsHistoryEmbeddingsEnabledForProfile(
          Profile::FromBrowserContext(browser_context))) {
    history_search_tool_ = std::make_unique<HistorySearchTool>(browser_context);
  }
#if BUILDFLAG(ENABLE_LOCAL_AI)
  if (AIChatEmbeddingsService* embeddings_service =
          AIChatEmbeddingsServiceFactory::GetForBrowserContext(
              browser_context)) {
    conversation_search_tool_ = std::make_unique<ConversationSearchTool>(
        embeddings_service->GetWeakPtr(), conversation_uuid_);
    memory_semantic_search_tool_ = std::make_unique<MemorySemanticSearchTool>(
        embeddings_service->GetWeakPtr());
  }
#endif
#if BUILDFLAG(ENABLE_AI_CHAT_TAB_MANAGEMENT_TOOL)
  if (base::FeatureList::IsEnabled(features::kTabManagementTool)) {
    tab_management_tool_ = std::make_unique<TabManagementTool>(profile_);
  }
#endif
}

}  // namespace ai_chat
