// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AGENT_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AGENT_CLIENT_H_

#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

class AgentClient : public Tool, public content::DevToolsAgentHostClient {
 public:
  explicit AgentClient(content::WebContents* web_contents);
  ~AgentClient() override;
  AgentClient(const AgentClient&) = delete;
  AgentClient& operator=(const AgentClient&) = delete;

  // Tool
  std::string_view name() const override;
  std::string_view description() const override;
  std::string_view type() const override;
  std::optional<std::string> GetInputSchemaJson() const override;
  void UseTool(const std::string& input_json,
               base::OnceCallback<void(std::optional<std::string_view>)> callback)
      override;

  // <message, error>
  using MessageResult = base::expected<std::string_view, std::string_view>;
  using MessageCallback =
      base::OnceCallback<void(MessageResult)>;

 private:
  void CaptureScreenshot(MessageCallback callback);

  void Execute(std::string_view method, std::string_view params, MessageCallback callback);

  // content::DevToolsAgentHostClient
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override;
  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;
  bool MayAttachToRenderFrameHost(
      content::RenderFrameHost* render_frame_host) override;
  bool IsTrusted() override;

  void OnMessageForToolUseComplete(
      base::OnceCallback<void(std::optional<std::string_view>)>
          tool_use_callback,
      MessageResult result);

  gfx::Point mouse_position_;

  int request_id_ = 1;
  std::map<int, MessageCallback> message_callbacks_;
  scoped_refptr<content::DevToolsAgentHost> devtools_agent_host_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AGENT_CLIENT_H_
