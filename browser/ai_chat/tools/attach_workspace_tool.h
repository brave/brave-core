// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_ATTACH_WORKSPACE_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_ATTACH_WORKSPACE_TOOL_H_

#include <string>
#include <string_view>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/types.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ai_chat {

class ConversationHandler;

// Lets the assistant attach a new, empty workspace to its conversation. The
// workspace (see WorkspaceAssociatedContent) is sent no folder, so it's backed
// by its own origin private file system, and contributes the file tools to the
// conversation. Replies once those tools are available to the current
// generation loop. Only offered while the conversation has no workspace.
class AttachWorkspaceTool : public Tool {
 public:
  // |conversation| owns this tool (via its tool provider), so outlives it.
  AttachWorkspaceTool(content::BrowserContext& browser_context,
                      ConversationHandler& conversation);
  ~AttachWorkspaceTool() override;

  AttachWorkspaceTool(const AttachWorkspaceTool&) = delete;
  AttachWorkspaceTool& operator=(const AttachWorkspaceTool&) = delete;

  // Tool:
  std::string_view Name() const override;
  std::string_view Description() const override;
  bool SupportsConversation(bool is_temporary,
                            bool has_untrusted_content,
                            const ConversationCapabilitySet&
                                conversation_capabilities) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  bool HasWorkspace() const;

  // Adds the tools of the workspace with |content_uuid| to the current
  // generation loop, then replies.
  void OnWorkspaceReady(const std::string& content_uuid,
                        UseToolCallback callback);

  const raw_ref<content::BrowserContext> browser_context_;
  const raw_ref<ConversationHandler> conversation_;

  base::WeakPtrFactory<AttachWorkspaceTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_ATTACH_WORKSPACE_TOOL_H_
