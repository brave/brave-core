// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AGENT_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AGENT_CLIENT_H_

#include "base/types/expected.h"
#include "brave/components/ai_chat/content/browser/ai_chat_cursor.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/accessibility/ax_tree_update.h"

namespace ai_chat {

// TODO(petemill): Rename to AnthropicComputerUseTool
class AgentClient : public Tool,
                    public content::DevToolsAgentHostClient,
                    public content::WebContentsObserver {
 public:
  explicit AgentClient(content::WebContents* web_contents);
  ~AgentClient() override;
  AgentClient(const AgentClient&) = delete;
  AgentClient& operator=(const AgentClient&) = delete;

  bool IsControllingContents() const {
    return !pending_navigation_callback_.is_null();
  }

  // Tool
  std::string_view name() const override;
  std::string_view description() const override;
  std::string_view type() const override;
  std::optional<base::Value::Dict> extra_params() const override;
  bool IsSupportedByModel(const mojom::Model& model) const override;
  void UseTool(const std::string& input_json,
               Tool::UseToolCallback callback) override;

  // content::WebContentsObserver
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFirstVisuallyNonEmptyPaint() override;

  // <message, error>
  using MessageResult = base::expected<std::string_view, std::string_view>;
  using MessageCallback = base::OnceCallback<void(MessageResult)>;

 private:
  void GetDomTree();
  void OnAXTreeSnapshot(ui::AXTreeUpdate& tree);
  void CaptureScreenshot(Tool::UseToolCallback callback);
  void TypeText(std::string_view text, Tool::UseToolCallback callback);
  void UpdateMousePosition(const gfx::Point& position);

  void Execute(std::string_view method,
               std::string_view params,
               MessageCallback callback);

  // content::DevToolsAgentHostClient
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;
  bool MayAttachToRenderFrameHost(
      content::RenderFrameHost* render_frame_host) override;
  bool IsTrusted() override;

  void PrepareForAgentActions();

  void PerformPossiblyNavigatingAction(
      base::OnceCallback<void(base::OnceClosure)> action,
      Tool::UseToolCallback callback);
  void MaybeFinishPossiblyNavigatingAction();

  gfx::Point mouse_position_;

  std::unique_ptr<AIChatCursorOverlay> cursor_overlay_;

  Tool::UseToolCallback pending_navigation_callback_;
  std::optional<bool> pending_navigation_complete_ = std::nullopt;
  bool pending_navigation_visually_painted_ = false;

  int request_id_ = 1;
  std::map<int, MessageCallback> message_callbacks_;
  scoped_refptr<content::DevToolsAgentHost> devtools_agent_host_;
  bool has_overriden_metrics_ = false;

  base::WeakPtrFactory<AgentClient> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AGENT_CLIENT_H_
