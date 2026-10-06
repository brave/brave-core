// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/attach_workspace_tool.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/browser/brave_tab_helpers.h"
#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {

AttachWorkspaceTool::AttachWorkspaceTool(
    content::BrowserContext* browser_context,
    ConversationHandler* conversation)
    : browser_context_(browser_context), conversation_(conversation) {
  CHECK(browser_context_);
  CHECK(conversation_);
}

AttachWorkspaceTool::~AttachWorkspaceTool() = default;

std::string_view AttachWorkspaceTool::Name() const {
  return mojom::kAttachWorkspaceToolName;
}

std::string_view AttachWorkspaceTool::Description() const {
  return "Creates a new, empty workspace and attaches it to this "
         "conversation. A workspace is a private folder of files that you can "
         "create, view, edit and search with file tools. Use this when the "
         "user asks you to write or work on files (for example code or "
         "documents) and the conversation has no workspace yet. The file "
         "tools become available from the user's next message, so tell the "
         "user the workspace is ready and ask them to continue.";
}

bool AttachWorkspaceTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    const ConversationCapabilitySet& conversation_capabilities) const {
  // A conversation only needs one workspace, and a second would register file
  // tools with the same names as the first.
  return !conversation_capabilities.contains(
      mojom::ConversationCapability::WORKSPACES);
}

void AttachWorkspaceTool::UseTool(const std::string& input_json,
                                  UseToolCallback callback) {
  // The tool is withheld once a workspace is attached, but one may have been
  // attached since the model was given the tool list.
  if (HasWorkspace()) {
    std::move(callback).Run(CreateContentBlocksForText(
                                "This conversation already has a workspace."),
                            {});
    return;
  }

  // A workspace's workspace:// URL isn't in |kAllowedContentSchemes|, so attach
  // it directly via the manager rather than
  // AIChatService::AssociateOwnedContent (which would reject the scheme).
  conversation_->associated_content_manager()->AddOwnedContent(
      std::make_unique<WorkspaceAssociatedContent>(
          /*folder_path=*/std::nullopt, browser_context_,
          base::BindOnce(&brave::AttachPrivacySensitiveTabHelpers)));

  // TODO(https://github.com/brave/brave-browser/issues/59734): Refresh the
  // conversation's content tools so the file tools can be used in this turn.
  std::move(callback).Run(
      CreateContentBlocksForText(
          "Attached an empty workspace to this conversation. Its file tools "
          "will be available from the user's next message."),
      {});
}

bool AttachWorkspaceTool::HasWorkspace() const {
  for (const auto& content :
       conversation_->associated_content_manager()->GetAssociatedContent()) {
    if (content->content_type == mojom::ContentType::Workspace) {
      return true;
    }
  }
  return false;
}

}  // namespace ai_chat
