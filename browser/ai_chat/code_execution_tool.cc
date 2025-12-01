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
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
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

}  // namespace

class CodeSandboxWebContentsObserver : public content::WebContentsObserver {
 public:
  using LoadCompleteCallback =
      base::OnceCallback<void(content::RenderFrameHost*)>;

  CodeSandboxWebContentsObserver(content::WebContents* web_contents,
                                 LoadCompleteCallback callback)
      : content::WebContentsObserver(web_contents),
        load_complete_callback_(std::move(callback)) {}

  ~CodeSandboxWebContentsObserver() override = default;

  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override {
    // If the frame has a parent, we can assume this is the iframe
    if (render_frame_host->GetParent() && load_complete_callback_) {
      base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, base::BindOnce(std::move(load_complete_callback_),
                                    render_frame_host));
    }
  }

 private:
  LoadCompleteCallback load_complete_callback_;
};

CodeExecutionTool::CodeExecutionRequest::CodeExecutionRequest(
    std::unique_ptr<content::WebContents> web_contents,
    std::string wrapped_js,
    UseToolCallback callback)
    : web_contents(std::move(web_contents)),
      wrapped_js(std::move(wrapped_js)),
      callback(std::move(callback)) {}

CodeExecutionTool::CodeExecutionRequest::~CodeExecutionRequest() = default;

CodeExecutionTool::CodeExecutionTool(content::BrowserContext* browser_context)
    : profile_(Profile::FromBrowserContext(browser_context)) {}

CodeExecutionTool::~CodeExecutionTool() = default;

std::string_view CodeExecutionTool::Name() const {
  return mojom::kCodeExecutionToolName;
}

std::string_view CodeExecutionTool::Description() const {
  return "Execute JavaScript code and return a human-readable formatted "
         "string as a result. "
         "Do not use console.log statements or similar statements. Always "
         "return a string as a result."
         "The code will be executed in a sandboxed environment.";
}

std::optional<base::Value::Dict> CodeExecutionTool::InputProperties() const {
  return CreateInputProperties(
      {{"script", StringProperty("The JavaScript code to execute")}});
}

std::optional<std::vector<std::string>> CodeExecutionTool::RequiredProperties()
    const {
  return std::vector<std::string>{"script"};
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
  execution_time_limit_for_testing_ = time_limit;
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

  const std::string* script = input_dict->FindString("script");

  if (!script || script->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'script' field"));
    return;
  }

  auto wrapped_js =
      base::StrCat({"(async function() { try {", *script,
                    "} catch (error) { return error.toString(); } })()"});

  auto* otr_profile =
      profile_->GetPrimaryOTRProfile(true /* create_if_needed */);
  content::WebContents::CreateParams create_params(otr_profile);
  auto web_contents = content::WebContents::Create(create_params);

  requests_.emplace_back(std::move(web_contents), wrapped_js,
                         std::move(callback));

  auto request_it = std::prev(requests_.end());
  request_it->observer = std::make_unique<CodeSandboxWebContentsObserver>(
      request_it->web_contents.get(),
      base::BindOnce(&CodeExecutionTool::OnPageLoadComplete,
                     weak_ptr_factory_.GetWeakPtr(), request_it));

  request_it->web_contents->GetController().LoadURL(
      GURL(kAIChatCodeSandboxUIURL), content::Referrer(),
      ui::PAGE_TRANSITION_TYPED, std::string());

  auto time_limit =
      execution_time_limit_for_testing_.value_or(kExecutionTimeLimit);
  request_it->timeout_timer.Start(
      FROM_HERE, time_limit,
      base::BindOnce(&CodeExecutionTool::ResolveRequest,
                     weak_ptr_factory_.GetWeakPtr(), request_it,
                     "Error: Time limit exceeded"));
}

void CodeExecutionTool::OnPageLoadComplete(
    std::list<CodeExecutionRequest>::iterator request_it,
    content::RenderFrameHost* render_frame_host) {
  render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(
      &request_it->injector);

  auto wrapped_js_utf16 = base::UTF8ToUTF16(request_it->wrapped_js);
  request_it->injector->RequestAsyncExecuteScript(
      content::ISOLATED_WORLD_ID_GLOBAL, wrapped_js_utf16,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::PromiseResultOption::kAwait,
      base::BindOnce(&CodeExecutionTool::HandleResult,
                     weak_ptr_factory_.GetWeakPtr(), request_it));
}

void CodeExecutionTool::HandleResult(
    std::list<CodeExecutionRequest>::iterator request_it,
    base::Value result) {
  std::string output;
  if (result.is_string()) {
    output = result.GetString();
  } else {
    output = "Error: Invalid return type";
  }
  ResolveRequest(request_it, output);
}

void CodeExecutionTool::ResolveRequest(
    std::list<CodeExecutionRequest>::iterator request_it,
    std::string output) {
  auto callback = std::move(request_it->callback);
  requests_.erase(request_it);
  std::move(callback).Run(CreateContentBlocksForText(output));
}

}  // namespace ai_chat
