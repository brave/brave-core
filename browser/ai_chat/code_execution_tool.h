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
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"

class Profile;

namespace content {
class WebContents;
class BrowserContext;
class RenderFrameHost;
}  // namespace content

namespace ai_chat {

class CodeSandboxWebContentsObserver;

// Result of code execution containing console output and optional chart image.
// Move-only because it may contain large base64-encoded image data.
struct ExecutionResult {
  ExecutionResult();
  ~ExecutionResult();
  ExecutionResult(ExecutionResult&&);
  ExecutionResult& operator=(ExecutionResult&&);

  // Concatenated console.log() output from the executed script.
  std::string console_output;
  // Base64-encoded PNG data URL (e.g., "data:image/png;base64,...") if the
  // script rendered a chart using window.createChart().
  std::optional<std::string> chart_image_data_url;
};

// Tool for executing JavaScript code in a sandboxed environment.
// Captures console.log output and optionally rendered chart images.
// The sandbox provides bignumber.js for decimal math and uPlot for charting.
// Network requests are blocked; execution is time-limited.
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
  class CodeExecutionRequest : public content::WebContentsObserver {
   public:
    using ResolveCallback = base::OnceCallback<void(ExecutionResult)>;

    CodeExecutionRequest(Profile* profile,
                         const std::string& script,
                         base::TimeDelta execution_time_limit);
    ~CodeExecutionRequest() override;

    void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                       const GURL& validated_url) override;

    void OnDidAddMessageToConsole(
        content::RenderFrameHost* source_frame,
        blink::mojom::ConsoleMessageLevel log_level,
        const std::u16string& message,
        int32_t line_no,
        const std::u16string& source_id,
        const std::optional<std::u16string>& untrusted_stack_trace) override;

    void SetResolveCallback(ResolveCallback callback) {
      resolve_callback_ = std::move(callback);
    }

   private:
    void HandleResult(base::Value result);
    void HandleTimeout();

    std::unique_ptr<content::WebContents> web_contents_;
    std::string wrapped_js_;
    mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> injector_;
    base::OneShotTimer timeout_timer_;
    ResolveCallback resolve_callback_;
    std::vector<std::string> console_logs_;
    base::WeakPtrFactory<CodeExecutionRequest> weak_ptr_factory_{this};
  };

  void ResolveRequest(std::list<CodeExecutionRequest>::iterator request_it,
                      UseToolCallback callback,
                      ExecutionResult result);

  raw_ptr<Profile> profile_;
  std::list<CodeExecutionRequest> requests_;
  base::TimeDelta execution_time_limit_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CODE_EXECUTION_TOOL_H_
