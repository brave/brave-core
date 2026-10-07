// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/attach_workspace_tool.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/task/bind_post_task.h"
#include "brave/browser/brave_tab_helpers.h"
#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace ai_chat {

namespace {

constexpr char kToolsAvailable[] =
    "Attached an empty workspace to this conversation. Its file tools are now "
    "available.";
constexpr char kToolsNextMessage[] =
    "Attached an empty workspace to this conversation, but its file tools "
    "aren't available yet. They will be from the user's next message, so tell "
    "the user the workspace is ready and ask them to continue.";

}  // namespace

AttachWorkspaceTool::AttachWorkspaceTool(
    content::BrowserContext& browser_context,
    ConversationHandler& conversation)
    : browser_context_(browser_context), conversation_(conversation) {}

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
         "tools are available once this returns.";
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

  auto workspace = std::make_unique<WorkspaceAssociatedContent>(
      /*folder_path=*/std::nullopt, &browser_context_.get(),
      base::BindOnce(&brave::AttachPrivacySensitiveTabHelpers));
  WorkspaceAssociatedContent* workspace_ptr = workspace.get();

  // A workspace's workspace:// URL isn't in |kAllowedContentSchemes|, so attach
  // it directly via the manager rather than
  // AIChatService::AssociateOwnedContent (which would reject the scheme).
  conversation_->associated_content_manager()->AddOwnedContent(
      std::move(workspace));

  // Content tools are otherwise only collected when a generation loop starts,
  // so reply once the workspace's tools have been added to this loop, letting
  // the model use them straight away. If the workspace is destroyed before its
  // page is ready, this still runs, and replies without them. It's posted, as
  // that happens while the manager is removing the workspace.
  workspace_ptr->RunWhenPageReady(mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      base::BindPostTaskToCurrentDefault(
          base::BindOnce(&AttachWorkspaceTool::OnWorkspaceReady,
                         weak_ptr_factory_.GetWeakPtr(), workspace_ptr->uuid(),
                         std::move(callback)))));
}

void AttachWorkspaceTool::OnWorkspaceReady(const std::string& content_uuid,
                                           UseToolCallback callback) {
  conversation_->associated_content_manager()->AddContentToolsToGenerationLoop(
      content_uuid,
      base::BindOnce(
          [](UseToolCallback callback, size_t tool_count) {
            std::move(callback).Run(
                CreateContentBlocksForText(tool_count > 0 ? kToolsAvailable
                                                          : kToolsNextMessage),
                {});
          },
          std::move(callback)));
}

bool AttachWorkspaceTool::HasWorkspace() const {
  const auto contents =
      conversation_->associated_content_manager()->GetAssociatedContent();
  return std::ranges::any_of(contents, [](const auto& content) {
    return content->content_type == mojom::ContentType::Workspace;
  });
}

}  // namespace ai_chat
