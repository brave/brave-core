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
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace ai_chat {

// Helper class to capture console messages from WebContents
class ConsoleMessageObserver : public content::WebContentsObserver {
 public:
  explicit ConsoleMessageObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  ~ConsoleMessageObserver() override = default;

  void OnDidAddMessageToConsole(
      content::RenderFrameHost* source_frame,
      blink::mojom::ConsoleMessageLevel log_level,
      const std::u16string& message,
      int32_t line_no,
      const std::u16string& source_id,
      const std::optional<std::u16string>& untrusted_stack_trace) override {
    console_messages_.push_back(base::UTF16ToUTF8(message));
  }

  std::string GetCombinedConsoleOutput() const {
    if (console_messages_.empty()) {
      return "(No console output)";
    }
    return base::JoinString(console_messages_, "\n");
  }

 private:
  std::vector<std::string> console_messages_;
};

CodeExecutionTool::CodeExecutionTool(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {}

CodeExecutionTool::~CodeExecutionTool() = default;

std::string_view CodeExecutionTool::Name() const {
  return mojom::kCodeExecutionToolName;
}

std::string_view CodeExecutionTool::Description() const {
  return "Execute JavaScript code and return console.log output. "
         "Use this tool to run JavaScript code snippets and see their output. "
         "The code will be executed in a sandboxed environment and any "
         "console.log statements will be captured and returned as the result.";
}

std::optional<base::Value::Dict> CodeExecutionTool::InputProperties() const {
  return CreateInputProperties(
      {{"program", StringProperty("The JavaScript code to execute")}});
}

std::optional<std::vector<std::string>> CodeExecutionTool::RequiredProperties()
    const {
  return std::vector<std::string>{"program"};
}

bool CodeExecutionTool::RequiresUserInteractionBeforeHandling() const {
  return false;
}

bool CodeExecutionTool::SupportsConversation(bool is_temporary,
                                             bool has_untrusted_content) const {
  // Support all conversation types for now
  return true;
}

void CodeExecutionTool::UseTool(const std::string& input_json,
                                UseToolCallback callback) {
  // Parse the input JSON
  auto input_dict = base::JSONReader::ReadDict(input_json);
  if (!input_dict.has_value()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Invalid JSON input, input must be a JSON object"));
    return;
  }

  const std::string* program = input_dict->FindString("program");

  if (!program || program->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'program' field"));
    return;
  }

  // Get the primary OTR profile for isolated execution
  Profile* profile = Profile::FromBrowserContext(browser_context_);
  Profile* otr_profile =
      profile->GetPrimaryOTRProfile(true /* create_if_needed */);

  // Create WebContents for JavaScript execution using the OTR profile
  content::WebContents::CreateParams create_params(otr_profile);
  web_contents_ = content::WebContents::Create(create_params);

  // Set up console message observer
  console_observer_ =
      std::make_unique<ConsoleMessageObserver>(web_contents_.get());

  // Load about:blank to ensure clean state
  web_contents_->GetController().LoadURL(
      GURL("about:blank"), content::Referrer(), ui::PAGE_TRANSITION_TYPED,
      std::string());

  // Wrap the JavaScript code in try-catch
  std::string wrapped_js =
      base::StrCat({"try {", "  ", *program, "\n", "} catch (error) {",
                    "  console.error('Execution error:', error);", "}"});

  // Execute JavaScript after a delay to ensure about:blank has loaded
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<CodeExecutionTool> tool, UseToolCallback callback,
             const std::string& js_code) {
            if (!tool || !tool->web_contents()) {
              std::move(callback).Run(CreateContentBlocksForText(
                  "Error: Tool was destroyed during execution"));
              return;
            }

            tool->web_contents()->GetPrimaryMainFrame()->ExecuteJavaScript(
                base::UTF8ToUTF16(js_code),
                base::BindOnce(
                    &CodeExecutionTool::OnJavaScriptExecutionComplete, tool,
                    std::move(callback)));
          },
          weak_ptr_factory_.GetWeakPtr(), std::move(callback), wrapped_js),
      base::Milliseconds(100));
}

void CodeExecutionTool::OnJavaScriptExecutionComplete(UseToolCallback callback,
                                                      base::Value result) {
  // Get console output from the observer
  std::string console_output;
  if (console_observer_) {
    console_output = console_observer_->GetCombinedConsoleOutput();
  }

  // Clean up the WebContents and observer after execution
  web_contents_.reset();
  console_observer_.reset();

  std::move(callback).Run(CreateContentBlocksForText(console_output));
}

}  // namespace ai_chat
