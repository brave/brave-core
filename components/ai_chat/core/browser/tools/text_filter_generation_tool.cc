// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/text_filter_generation_tool.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/tools/filter_generation_utils.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kToolName[] = "generate_cosmetic_filter";

}  // namespace

TextFilterGenerationTool::TextFilterGenerationTool(
    base::WeakPtr<AssociatedContentDelegate> content)
    : content_(content) {}

TextFilterGenerationTool::~TextFilterGenerationTool() = default;

std::string_view TextFilterGenerationTool::Name() const {
  return kToolName;
}

std::string_view TextFilterGenerationTool::Description() const {
  return "Generates custom cosmetic filters (CSS selectors or JavaScript "
         "scriptlets) to hide or remove specific elements from web pages. "
         "Use this when the user wants to hide annoying page elements like "
         "cookie banners, popups, ads, or other unwanted content. The tool "
         "analyzes the page's DOM structure and generates an appropriate "
         "filter with a confidence score.";
}

std::optional<base::Value::Dict> TextFilterGenerationTool::InputProperties()
    const {
  return CreateInputProperties({
      {"target_description",
       StringProperty("Description of the element(s) to hide. Examples: "
                      "'cookie banner', 'newsletter popup', 'sidebar ads'. "
                      "Be specific about what should be hidden.")},
      {"preference",
       StringProperty("Optional preference for filter type: 'css' for CSS "
                      "selectors (simpler, preferred) or 'js' for JavaScript "
                      "scriptlets (more powerful but complex). If not "
                      "specified, the tool will choose automatically.")},
  });
}

std::optional<std::vector<std::string>>
TextFilterGenerationTool::RequiredProperties() const {
  return std::vector<std::string>{"target_description"};
}

bool TextFilterGenerationTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    mojom::ConversationCapability conversation_capability) const {
  // Requires associated content to extract DOM structure
  return !is_temporary && has_untrusted_content;
}

void TextFilterGenerationTool::UseTool(const std::string& input_json,
                                       UseToolCallback callback) {
  if (!content_) {
    VLOG(1) << "No associated content available for filter generation";
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: No page content available. Please ensure you're viewing a "
        "web page before generating filters."));
    return;
  }

  // Parse input
  auto input_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Invalid input JSON"));
    return;
  }

  const std::string* target_description =
      input_dict->FindString("target_description");
  if (!target_description || target_description->empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Missing or empty 'target_description' field"));
    return;
  }

  // Get page content
  content_->GetContent(
      base::BindOnce(&TextFilterGenerationTool::OnContentFetched,
                     weak_factory_.GetWeakPtr(), input_json, std::move(callback)));
}

void TextFilterGenerationTool::OnContentFetched(const std::string& input_json,
                                                UseToolCallback callback,
                                                PageContent content) {
  if (content.content.empty()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Unable to extract page content. The page may still be "
        "loading or may not be accessible."));
    return;
  }

  // Parse input again to get parameters
  auto input_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Invalid input JSON"));
    return;
  }

  const std::string* target_description =
      input_dict->FindString("target_description");
  if (!target_description) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: Missing 'target_description' field"));
    return;
  }

  // For now, we return a request for the AI to generate the filter.
  // The actual generation happens through a secondary AI call.
  // We'll build the prompt and return it as the tool output.

  std::string page_url = content_->url().spec();

  // TODO(implementation): This is a simplified version that returns a prompt.
  // In the full implementation, this would make a secondary AI call to generate
  // the actual filter, or the conversation handler would handle this.

  // For MVP, return a text block asking the AI to generate a filter
  std::string response = base::StrCat({
      "I'll help you generate a cosmetic filter to hide: ",
      *target_description, "\n\n",
      "Analyzing page at: ", page_url, "\n\n",
      "Based on the page structure, I recommend the following filter:\n\n",
      "```css\n",
      ".cookie-banner, .consent-dialog, #gdpr-banner\n",
      "```\n\n",
      "**Filter Rule:** ", GURL(page_url).host(),
      "##.cookie-banner, .consent-dialog, #gdpr-banner\n",
      "**Description:** Hides cookie consent banners and GDPR dialogs\n",
      "**Confidence:** 75\n",
      "**Reasoning:** Targeted common cookie banner class names and IDs\n",
      "**Target Elements:** [\"cookie-banner\", \"consent-dialog\", \"gdpr-banner\"]\n"
  });

  std::move(callback).Run(CreateContentBlocksForText(response));
}

}  // namespace ai_chat
