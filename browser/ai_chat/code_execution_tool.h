// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CODE_EXECUTION_TOOL_H_
#define BRAVE_BROWSER_AI_CHAT_CODE_EXECUTION_TOOL_H_

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

class Profile;

namespace content {
class WebContents;
class BrowserContext;
class RenderFrameHost;
}  // namespace content

namespace ai_chat {

class CodeSandboxWebContentsObserver;

// Tool for executing JavaScript code and returning console.log output.
// This tool is provided by the browser and allows AI assistants to run
// JavaScript code in a sandboxed environment.
class CodeExecutionTool : public Tool {
 public:
  explicit CodeExecutionTool(content::BrowserContext* browser_context);
  ~CodeExecutionTool() override;

  CodeExecutionTool(const CodeExecutionTool&) = delete;
  CodeExecutionTool& operator=(const CodeExecutionTool&) = delete;

  // Tool overrides
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  bool SupportsConversation(
      bool is_temporary,
      bool has_untrusted_content,
      mojom::ConversationCapability conversation_capability) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

  void SetExecutionTimeLimitForTesting(base::TimeDelta time_limit);

 private:
  struct CodeExecutionRequest {
    CodeExecutionRequest(std::unique_ptr<content::WebContents> web_contents,
                         std::string wrapped_js,
                         UseToolCallback callback);
    ~CodeExecutionRequest();

    std::unique_ptr<content::WebContents> web_contents;
    std::string wrapped_js;
    std::unique_ptr<CodeSandboxWebContentsObserver> observer;
    mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> injector;
    base::OneShotTimer timeout_timer;
    UseToolCallback callback;
  };

  void OnPageLoadComplete(std::list<CodeExecutionRequest>::iterator request_it,
                          content::RenderFrameHost* render_frame_host);
  void HandleResult(std::list<CodeExecutionRequest>::iterator request_it,
                    base::Value result);
  void ResolveRequest(std::list<CodeExecutionRequest>::iterator request_it,
                      std::string output);

  raw_ptr<Profile> profile_;
  std::list<CodeExecutionRequest> requests_;
  std::optional<base::TimeDelta> execution_time_limit_for_testing_;
  base::WeakPtrFactory<CodeExecutionTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CODE_EXECUTION_TOOL_H_
