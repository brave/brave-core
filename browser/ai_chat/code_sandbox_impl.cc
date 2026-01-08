// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/code_sandbox_impl.h"

#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/mojom/script/script_evaluation_params.mojom.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr base::TimeDelta kExecutionTimeLimit = base::Seconds(10);

std::string WrapScript(const std::string& script) {
  return base::StrCat({"(async function() { try { ", script,
                       " } catch (error) { console.error(error.toString()); } "
                       "return true; })()"});
}

}  // namespace

CodeSandboxImpl::CodeExecutionRequest::CodeExecutionRequest(
    Profile* profile,
    const std::string& script,
    base::TimeDelta execution_time_limit)
    : content::WebContentsObserver(nullptr), wrapped_js_(WrapScript(script)) {
  auto otr_profile_id = Profile::OTRProfileID::AIChatCodeExecutionID();
  auto* otr_profile = profile->GetOffTheRecordProfile(
      otr_profile_id, /*create_if_needed=*/true);
  content::WebContents::CreateParams create_params(otr_profile);
  web_contents_ = content::WebContents::Create(create_params);

  Observe(web_contents_.get());

  web_contents_->GetController().LoadURL(
      GURL(kAIChatCodeSandboxUIURL), content::Referrer(),
      ui::PAGE_TRANSITION_TYPED, std::string());

  timeout_timer_.Start(FROM_HERE, execution_time_limit,
                       base::BindOnce(&CodeExecutionRequest::HandleTimeout,
                                      base::Unretained(this)));
}

CodeSandboxImpl::CodeExecutionRequest::~CodeExecutionRequest() {
  Observe(nullptr);
}

void CodeSandboxImpl::CodeExecutionRequest::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (!render_frame_host->GetParent() || wrapped_js_.empty()) {
    return;
  }

  render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(&injector_);

  auto wrapped_js_utf16 = base::UTF8ToUTF16(wrapped_js_);

  // Clear the wrapped script to avoid re-using it.
  wrapped_js_ = {};

  injector_->RequestAsyncExecuteScript(
      content::ISOLATED_WORLD_ID_GLOBAL, wrapped_js_utf16,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::PromiseResultOption::kAwait,
      base::BindOnce(&CodeExecutionRequest::HandleResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CodeSandboxImpl::CodeExecutionRequest::OnDidAddMessageToConsole(
    content::RenderFrameHost* source_frame,
    blink::mojom::ConsoleMessageLevel log_level,
    const std::u16string& message,
    int32_t line_no,
    const std::u16string& source_id,
    const std::optional<std::u16string>& untrusted_stack_trace) {
  console_logs_.push_back(base::UTF16ToUTF8(message));
}

void CodeSandboxImpl::CodeExecutionRequest::HandleResult(base::Value result) {
  if (!result.is_bool() || !result.GetBool()) {
    std::move(resolve_callback_).Run("Error: Syntax error");
    return;
  }

  std::move(resolve_callback_).Run(base::JoinString(console_logs_, "\n"));
}

void CodeSandboxImpl::CodeExecutionRequest::HandleTimeout() {
  std::move(resolve_callback_).Run("Error: Time limit exceeded");
}

void CodeSandboxImpl::ResolveRequest(
    std::list<CodeExecutionRequest>::iterator request_it,
    ExecuteCodeCallback callback,
    std::string output) {
  requests_.erase(request_it);
  std::move(callback).Run(std::move(output));
}

CodeSandboxImpl::CodeSandboxImpl(content::BrowserContext* browser_context)
    : profile_(Profile::FromBrowserContext(browser_context)),
      execution_time_limit_(kExecutionTimeLimit) {}

CodeSandboxImpl::~CodeSandboxImpl() = default;

void CodeSandboxImpl::SetExecutionTimeLimitForTesting(
    base::TimeDelta time_limit) {
  execution_time_limit_ = time_limit;
}

void CodeSandboxImpl::ExecuteCode(const std::string& script,
                                  ExecuteCodeCallback callback) {
  requests_.emplace_back(profile_, script, execution_time_limit_);

  auto request_it = std::prev(requests_.end());
  request_it->SetResolveCallback(
      base::BindOnce(&CodeSandboxImpl::ResolveRequest, base::Unretained(this),
                     request_it, std::move(callback)));
}

}  // namespace ai_chat
