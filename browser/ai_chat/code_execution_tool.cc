// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/code_execution_tool.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
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
constexpr char kScriptProperty[] = "script";

std::string WrapScript(const std::string& script) {
  return base::StrCat({"(async function() { try { ", script,
                       " } catch (error) { return error.toString(); } })()"});
}

}  // namespace

CodeExecutionTool::CodeExecutionRequest::CodeExecutionRequest(
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

CodeExecutionTool::CodeExecutionRequest::~CodeExecutionRequest() {
  Observe(nullptr);
}

void CodeExecutionTool::CodeExecutionRequest::DidFinishLoad(
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

void CodeExecutionTool::CodeExecutionRequest::HandleResult(base::Value result) {
  std::string output;
  if (result.is_string()) {
    output = result.GetString();
  } else {
    output = "Error: Invalid return type or syntax error";
  }
  std::move(resolve_callback_).Run(output);
}

void CodeExecutionTool::CodeExecutionRequest::HandleTimeout() {
  std::move(resolve_callback_).Run("Error: Time limit exceeded");
}

void CodeExecutionTool::ResolveRequest(
    std::list<CodeExecutionRequest>::iterator request_it,
    UseToolCallback callback,
    std::string output) {
  requests_.erase(request_it);
  std::move(callback).Run(CreateContentBlocksForText(output));
}

CodeExecutionTool::CodeExecutionTool(content::BrowserContext* browser_context)
    : profile_(Profile::FromBrowserContext(browser_context)),
      execution_time_limit_(kExecutionTimeLimit) {}

CodeExecutionTool::~CodeExecutionTool() = default;

std::string_view CodeExecutionTool::Name() const {
  return mojom::kCodeExecutionToolName;
}

std::string_view CodeExecutionTool::Description() const {
  return "Execute JavaScript code and return a human-readable formatted "
         "string as a result. "
         "Use only when the task warrants actual code execution or processing. "
         "Do not use this for content generation. "
         "Do not use this for fetching information from the internet. "
         "Do not use console.log statements or similar statements. Always "
         "return a string as a result. Always use an explicit return statement "
         "(i.e. return result). "
         "The code will be executed in a sandboxed environment."
         "Network requests are not allowed.";
}

std::optional<base::Value::Dict> CodeExecutionTool::InputProperties() const {
  return CreateInputProperties(
      {{kScriptProperty, StringProperty("The JavaScript code to execute")}});
}

std::optional<std::vector<std::string>> CodeExecutionTool::RequiredProperties()
    const {
  return std::vector<std::string>{kScriptProperty};
}

std::variant<bool, mojom::PermissionChallengePtr>
CodeExecutionTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  return false;
}

bool CodeExecutionTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    mojom::ConversationCapability conversation_capability) const {
  // Support all conversation types for now
  return true;
}

void CodeExecutionTool::SetExecutionTimeLimitForTesting(
    base::TimeDelta time_limit) {
  execution_time_limit_ = time_limit;
}

void CodeExecutionTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  auto input_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Invalid JSON input, input must be a JSON object"));
    return;
  }

  const std::string* script = input_dict->FindString(kScriptProperty);

  if (!script || script->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'script' field"));
    return;
  }

  requests_.emplace_back(profile_, *script, execution_time_limit_);

  auto request_it = std::prev(requests_.end());
  request_it->SetResolveCallback(
      base::BindOnce(&CodeExecutionTool::ResolveRequest, base::Unretained(this),
                     request_it, std::move(callback)));
}

}  // namespace ai_chat
