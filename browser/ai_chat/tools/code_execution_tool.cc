// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/code_execution_tool.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/ai_chat/core/browser/tools/chart_code_plugin.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/mojom/script/script_evaluation_params.mojom.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr base::TimeDelta kExecutionTimeLimit = base::Seconds(10);
constexpr char kScriptProperty[] = "script";
constexpr char kArtifactTypeKey[] = "type";
constexpr char kArtifactContentKey[] = "content";

}  // namespace

CodeExecutionTool::CodeExecutionRequest::CodeExecutionRequest(
    Profile* profile,
    std::string script,
    base::TimeDelta execution_time_limit)
    : content::WebContentsObserver(nullptr), script_(std::move(script)) {
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
  if (!render_frame_host->GetParent() || script_.empty()) {
    return;
  }

  render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(&injector_);

  auto script_utf16 = base::UTF8ToUTF16(script_);

  // Clear the wrapped script to avoid re-using it.
  script_ = {};

  injector_->RequestAsyncExecuteScript(
      content::ISOLATED_WORLD_ID_GLOBAL, script_utf16,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::PromiseResultOption::kAwait,
      base::BindOnce(&CodeExecutionRequest::HandleResult,
                     weak_ptr_factory_.GetWeakPtr()));
}

void CodeExecutionTool::CodeExecutionRequest::OnDidAddMessageToConsole(
    content::RenderFrameHost* source_frame,
    blink::mojom::ConsoleMessageLevel log_level,
    const std::u16string& message,
    int32_t line_no,
    const std::u16string& source_id,
    const std::optional<std::u16string>& untrusted_stack_trace) {
  console_logs_.push_back(base::UTF16ToUTF8(message));
}

void CodeExecutionTool::CodeExecutionRequest::HandleResult(base::Value result) {
  if (!result.is_list()) {
    std::move(resolve_callback_).Run("Error: Syntax error", {});
    return;
  }

  std::string console_logs_str = base::JoinString(console_logs_, "\n");
  std::move(resolve_callback_)
      .Run(std::move(console_logs_str), std::move(result).TakeList());
}

void CodeExecutionTool::CodeExecutionRequest::HandleTimeout() {
  std::move(resolve_callback_).Run("Error: Time limit exceeded", {});
}

void CodeExecutionTool::ResolveRequest(
    std::list<CodeExecutionRequest>::iterator request_it,
    UseToolCallback callback,
    std::string console_logs,
    base::Value::List artifacts) {
  requests_.erase(request_it);

  std::vector<mojom::ToolArtifactPtr> artifact_ptrs;
  std::optional<std::string> error;

  // Process artifacts
  for (const auto& artifact : artifacts) {
    const auto* artifact_dict = artifact.GetIfDict();
    if (!artifact_dict) {
      error = "Error: Artifact must be an object";
      break;
    }

    const auto* type = artifact_dict->FindString(kArtifactTypeKey);
    const auto* content = artifact_dict->Find(kArtifactContentKey);
    if (!type || !content) {
      error = "Error: Artifact missing required 'type' or 'content' field";
      break;
    }

    // Find matching plugin and validate artifact
    bool plugin_found = false;
    for (const auto& plugin : code_plugins_) {
      if (plugin->ArtifactType() != *type) {
        continue;
      }
      plugin_found = true;
      if (auto validation_error = plugin->ValidateArtifact(*content)) {
        error = base::StrCat({"Error: ", *validation_error});
      }
      break;
    }

    if (!plugin_found) {
      error =
          base::StrCat({"Error: Artifact type '", *type, "' is not supported"});
      break;
    }

    if (error) {
      break;
    }

    // Serialize content to JSON string for storage
    std::string content_json;
    if (!base::JSONWriter::Write(*content, &content_json)) {
      error = "Error: Failed to serialize artifact content";
      break;
    }

    // Add artifact
    artifact_ptrs.push_back(
        mojom::ToolArtifact::New(*type, std::move(content_json)));
  }

  // Construct final content blocks
  std::vector<mojom::ContentBlockPtr> content_blocks;

  // If error occurred, use error message instead of console logs
  if (error) {
    content_blocks = CreateContentBlocksForText(std::move(*error));
    artifact_ptrs.clear();
  } else {
    content_blocks = CreateContentBlocksForText(std::move(console_logs));
  }

  std::move(callback).Run(std::move(content_blocks), std::move(artifact_ptrs));
}

CodeExecutionTool::CodeExecutionTool(content::BrowserContext* browser_context)
    : profile_(Profile::FromBrowserContext(browser_context)),
      execution_time_limit_(kExecutionTimeLimit) {
  if (ChartCodePlugin::IsEnabled()) {
    code_plugins_.push_back(std::make_unique<ChartCodePlugin>());
  }

  // Build the description with plugin information
  std::vector<std::string_view> plugin_descriptions;
  for (const auto& plugin : code_plugins_) {
    plugin_descriptions.emplace_back(plugin->Description());
  }

  tool_description_ = base::StrCat(
      {"Execute JavaScript code and capture console output. "
       "Use only when the task requires code execution for providing an "
       "accurate answer. "
       "Do not use this if you are able to answer without executing code. "
       "Do not use this for content generation. "
       "Do not use this for fetching information from the internet. "
       "Use console.log() to output results. "
       "The code will be executed in a sandboxed environment. "
       "Network requests are not allowed. "
       "bignumber.js is available in the global scope. Use it for any "
       "decimal math (i.e. financial calculations). "
       "Do not use require to import bignumber.js, as it is not needed. ",
       base::JoinString(plugin_descriptions, " "),
       "\nExample tasks that require code execution:\n"
       " - Financial calculations (e.g. compound interest)\n"
       " - Analyzing data or web content\n"
       "Example tasks that do not require code execution:\n"
       " - Very simple calculations (e.g. 2 + 2)\n"
       " - Finding the 4th prime number\n"
       " - Retrieving weather information for a location"});
}

CodeExecutionTool::~CodeExecutionTool() = default;

std::string_view CodeExecutionTool::Name() const {
  return mojom::kCodeExecutionToolName;
}

std::string_view CodeExecutionTool::Description() const {
  return tool_description_;
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

std::string CodeExecutionTool::WrapScript(const std::string& script) const {
  auto bignumber_js =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_AI_CHAT_BIGNUMBER_JS);

  std::vector<std::string_view> plugin_scripts;
  for (const auto& plugin : code_plugins_) {
    if (script.find(plugin->InclusionKeyword()) != std::string::npos) {
      plugin_scripts.push_back(plugin->SetupScript());
    }
  }

  return base::StrCat({"(async function() { let codeExecArtifacts = []; ",
                       bignumber_js, base::StrCat(plugin_scripts), " try { ",
                       script,
                       " } catch (error) { console.error(error.toString()); } "
                       "return codeExecArtifacts; })()"});
}

void CodeExecutionTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  auto input_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Error: Invalid JSON input, input must be a JSON object"),
        {});
    return;
  }

  const std::string* script = input_dict->FindString(kScriptProperty);

  if (!script || script->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'script' field"),
        {});
    return;
  }

  std::string wrapped_script = WrapScript(*script);
  requests_.emplace_back(profile_, std::move(wrapped_script),
                         execution_time_limit_);

  auto request_it = std::prev(requests_.end());
  request_it->SetResolveCallback(
      base::BindOnce(&CodeExecutionTool::ResolveRequest, base::Unretained(this),
                     request_it, std::move(callback)));
}

}  // namespace ai_chat
