// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/text_filter_generation_tool.h"

#include <iostream>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/tools/filter_generation_utils.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

TextFilterGenerationTool::TextFilterGenerationTool(
    AssociatedContentManager* associated_content_manager)
    : associated_content_manager_(associated_content_manager) {
  CHECK(associated_content_manager);
  std::cerr << "\n[FILTER_TOOL] TextFilterGenerationTool constructed!\n";
}

TextFilterGenerationTool::~TextFilterGenerationTool() {
  std::cerr << "[FILTER_TOOL] TextFilterGenerationTool destroyed\n";
}

std::string_view TextFilterGenerationTool::Name() const {
  std::cerr << "[FILTER_TOOL] Name() called, returning: text_filter_generation\n";
  return "text_filter_generation";
}

std::string_view TextFilterGenerationTool::Description() const {
  std::cerr << "[FILTER_TOOL] Description() called\n";
  return "Use this tool when the user wants to modify the webpage. "
         "Call this tool for requests like: "
         "'hide the page title', 'remove the cookie banner', 'hide the popup', "
         "change the color of the background, "
         "'block the sidebar', or any request to modify page elements. "
         "This tool generates JavaScript custom scriptlets to manipulate the page DOM. "
         "IMPORTANT: After using this tool, you MUST respond following the exact "
         "format specified in the tool output, with markdown formatting including "
         "a reasoning explanation, code blocks for JavaScript, scriptlet name, and "
         "filter rule. Do NOT return JSON - return formatted markdown as shown in "
         "the example.";
}

std::optional<base::Value::Dict> TextFilterGenerationTool::InputProperties()
    const {
  std::cerr << "[FILTER_TOOL] InputProperties() called\n";
  return CreateInputProperties({
      {"user_request",
       StringProperty("What the user wants to modify on the "
                      "page. Examples: 'cookie banner', 'popup', 'sidebar'")},
  });
}

std::optional<std::vector<std::string>>
TextFilterGenerationTool::RequiredProperties() const {
  std::cerr << "[FILTER_TOOL] RequiredProperties() called\n";
  return std::vector<std::string>{"user_request"};
}

bool TextFilterGenerationTool::IsSupportedByModel(
    const mojom::Model& model) const {
  std::cerr << "[FILTER_TOOL] IsSupportedByModel() called - model.supports_tools="
            << model.supports_tools << "\n";
  // This tool works with any model since it just generates a prompt
  // that the model then processes. We don't need formal tool calling support.
  std::cerr << "[FILTER_TOOL] IsSupportedByModel() returning: true (overridden)\n";
  return true;
}

bool TextFilterGenerationTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    mojom::ConversationCapability conversation_capability) const {
  std::cerr << "[FILTER_TOOL] SupportsConversation() called - "
            << "is_temporary=" << is_temporary
            << ", has_untrusted_content=" << has_untrusted_content
            << ", conversation_capability=" << static_cast<int>(conversation_capability)
            << "\n";
  // Only available when we have associated content (a page to analyze)
  bool supports = has_untrusted_content;
  std::cerr << "[FILTER_TOOL] SupportsConversation() returning: " << supports << "\n";
  return supports;
}

void TextFilterGenerationTool::UseTool(const std::string& input_json,
                                       UseToolCallback callback) {
  std::cerr << "\n\n";
  std::cerr << "========================================\n";
  std::cerr << "[FILTER_TOOL] *** USETOOL CALLED! ***\n";
  std::cerr << "========================================\n";
  std::cerr << "[FILTER_TOOL] Input JSON: " << input_json << "\n";

  // Parse input JSON
  std::optional<base::Value> parsed = base::JSONReader::Read(input_json);
  if (!parsed || !parsed->is_dict()) {
    std::cerr << "ERROR: Failed to parse input JSON\n";
    VLOG(1) << "TextFilterGenerationTool: Failed to parse input JSON";
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Invalid input - Failed to parse JSON"));
    return;
  }

  const base::Value::Dict& input_dict = parsed->GetDict();
  const std::string* user_request = input_dict.FindString("user_request");

  if (!user_request || user_request->empty()) {
    std::cerr << "ERROR: Missing or empty user_request field\n";
    VLOG(1) << "TextFilterGenerationTool: Missing user_request";
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Invalid input - Missing user_request field"));
    return;
  }

  std::cerr << "User request: " << *user_request << "\n";

  // Get associated content and DOM structure
  auto associated_content_vec = associated_content_manager_->GetAssociatedContent();
  auto cached_contents = associated_content_manager_->GetCachedContents();

  if (associated_content_vec.empty() || cached_contents.empty()) {
    std::cerr << "ERROR: No associated content available\n";
    VLOG(1) << "TextFilterGenerationTool: No associated content";
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: No page content available to analyze. Please ensure you're viewing "
        "a webpage."));
    return;
  }

  // Get domain and URL from the first content
  std::string domain = associated_content_vec[0]->url.host();
  std::string url = associated_content_vec[0]->url.spec();
  std::cerr << "Domain: " << domain << "\n";
  std::cerr << "URL: " << url << "\n";

  // Get page content with DOM structure
  const auto& page_content = cached_contents[0].get();

  if (!page_content.dom_structure.has_value()) {
    std::cerr << "ERROR: No DOM structure available\n";
    VLOG(1) << "TextFilterGenerationTool: No DOM structure";
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Unable to analyze page structure. DOM structure not available."));
    return;
  }

  std::cerr << "DOM structure size: " << page_content.dom_structure->length()
            << " chars\n";

  // Build the filter generation prompt with URL included
  std::string filter_prompt = BuildFilterGenerationPrompt(
      page_content.content, *page_content.dom_structure, *user_request, domain, url);

  std::cerr << "\n=== FILTER GENERATION PROMPT ===\n"
            << filter_prompt << "\n"
            << "=== END PROMPT ===\n";

  // Return the prompt directly as plain text (not wrapped in JSON)
  // This way the AI treats it as instructions to follow, not data to describe
  std::string tool_output = filter_prompt;

  std::cerr << "Tool output length: " << tool_output.length() << " chars\n";
  std::cerr << "=== END TEXT FILTER GENERATION TOOL ===\n\n";

  // Create tool output using utility function
  std::move(callback).Run(CreateContentBlocksForText(tool_output));
}

}  // namespace ai_chat
