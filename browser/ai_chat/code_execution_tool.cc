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
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr base::TimeDelta kExecutionTimeLimit = base::Seconds(10);

}  // namespace

class CodeSandboxWebContentsObserver : public content::WebContentsObserver {
 public:
  explicit CodeSandboxWebContentsObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  ~CodeSandboxWebContentsObserver() override = default;

  void OnDidAddMessageToConsole(
      content::RenderFrameHost* source_frame,
      blink::mojom::ConsoleMessageLevel log_level,
      const std::u16string& message,
      int32_t line_no,
      const std::u16string& source_id,
      const std::optional<std::u16string>& untrusted_stack_trace) override {
    console_messages_.push_back(base::UTF16ToUTF8(message));
  }

  std::string GetConsoleContents() const {
    return base::JoinString(console_messages_, "\n");
  }

 private:
  std::vector<std::string> console_messages_;
};

CodeExecutionTool::CodeExecutionRequest::CodeExecutionRequest(
    std::unique_ptr<content::WebContents> web_contents,
    std::unique_ptr<CodeSandboxWebContentsObserver> observer,
    raw_ptr<Profile> otr_profile,
    UseToolCallback callback)
    : web_contents(std::move(web_contents)),
      observer(std::move(observer)),
      otr_profile(otr_profile),
      callback(std::move(callback)) {}

CodeExecutionTool::CodeExecutionRequest::~CodeExecutionRequest() = default;

CodeExecutionTool::CodeExecutionTool(content::BrowserContext* browser_context)
    : profile_(Profile::FromBrowserContext(browser_context)) {}

CodeExecutionTool::~CodeExecutionTool() {
  for (auto& request : requests_) {
    if (request.otr_profile) {
      profile_->DestroyOffTheRecordProfile(request.otr_profile);
    }
  }
}

std::string_view CodeExecutionTool::Name() const {
  return mojom::kCodeExecutionToolName;
}

std::string_view CodeExecutionTool::Description() const {
  return "Execute JavaScript code for performing calculations/computations. "
         "Ensure snippet ends with return statement. Always return a string. "
         "Do not use console logging statements.";
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
      base::StrCat({"(function(window, location) { try {", *script,
                    "} catch (error) {console.error('Error:', "
                    "error.toString()); } })(undefined, undefined)"});

  auto otr_profile_id = Profile::OTRProfileID::CreateUniqueForCodeSandbox();
  auto* otr_profile = profile_->GetOffTheRecordProfile(
      otr_profile_id, true /* create_if_needed */);

  content::WebContents::CreateParams create_params(otr_profile);
  auto web_contents = content::WebContents::Create(create_params);

  requests_.emplace_back(std::move(web_contents), nullptr, otr_profile,
                         std::move(callback));

  auto request_it = std::prev(requests_.end());
  request_it->observer = std::make_unique<CodeSandboxWebContentsObserver>(
      request_it->web_contents.get());

  auto* rfh = request_it->web_contents->GetPrimaryMainFrame();
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&request_it->injector);

  request_it->web_contents->GetController().LoadURL(
      GURL(kAIChatCodeSandboxUIURL), content::Referrer(),
      ui::PAGE_TRANSITION_TYPED, std::string());

  wrapped_js = "(function() { return 'Hello, world!'; })()";
  auto wrapped_js_utf16 = base::UTF8ToUTF16(wrapped_js);
  request_it->injector->RequestAsyncExecuteScript(
      ISOLATED_WORLD_ID_BRAVE_INTERNAL, wrapped_js_utf16,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::PromiseResultOption::kAwait,
      base::BindOnce(&CodeExecutionTool::HandleScriptResult,
                     weak_ptr_factory_.GetWeakPtr(), request_it));

  auto time_limit =
      execution_time_limit_for_testing_.value_or(kExecutionTimeLimit);
  request_it->timeout_timer.Start(
      FROM_HERE, time_limit,
      base::BindOnce(&CodeExecutionTool::HandleTimeout,
                     weak_ptr_factory_.GetWeakPtr(), request_it));
}

void CodeExecutionTool::HandleScriptResult(
    std::list<CodeExecutionRequest>::iterator request_it,
    base::Value result) {
  LOG(ERROR) << "HandleScriptResult: " << result.DebugString();
  if (!result.is_string()) {
    HandleResult(request_it, "Error: Invalid result type");
    return;
  }
  HandleResult(request_it, result.GetString());
}

void CodeExecutionTool::HandleTimeout(
    std::list<CodeExecutionRequest>::iterator request_it) {
  HandleResult(request_it, "Error: Time limit exceeded");
}

void CodeExecutionTool::HandleResult(
    std::list<CodeExecutionRequest>::iterator request_it,
    std::string output) {
  auto callback = std::move(request_it->callback);
  Profile* otr_profile = request_it->otr_profile;

  requests_.erase(request_it);

  if (otr_profile) {
    profile_->DestroyOffTheRecordProfile(otr_profile);
  }

  std::move(callback).Run(CreateContentBlocksForText(output));
}

}  // namespace ai_chat
