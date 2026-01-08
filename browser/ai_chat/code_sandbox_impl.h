// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CODE_SANDBOX_IMPL_H_
#define BRAVE_BROWSER_AI_CHAT_CODE_SANDBOX_IMPL_H_

#include <list>
#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/code_sandbox.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom.h"

class Profile;

namespace content {
class BrowserContext;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace ai_chat {

// Implementation of CodeSandbox that executes JavaScript
// code in an isolated WebContents environment.
class CodeSandboxImpl : public CodeSandbox {
 public:
  explicit CodeSandboxImpl(content::BrowserContext* browser_context);
  ~CodeSandboxImpl() override;

  CodeSandboxImpl(const CodeSandboxImpl&) = delete;
  CodeSandboxImpl& operator=(const CodeSandboxImpl&) = delete;

  // CodeSandbox implementation
  void ExecuteCode(const std::string& script,
                   ExecuteCodeCallback callback) override;

  void SetExecutionTimeLimitForTesting(base::TimeDelta time_limit);

 private:
  class CodeExecutionRequest : public content::WebContentsObserver {
   public:
    using ResolveCallback = base::OnceCallback<void(std::string)>;

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
                      ExecuteCodeCallback callback,
                      std::string output);

  raw_ptr<Profile> profile_;
  std::list<CodeExecutionRequest> requests_;
  base::TimeDelta execution_time_limit_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CODE_SANDBOX_IMPL_H_
