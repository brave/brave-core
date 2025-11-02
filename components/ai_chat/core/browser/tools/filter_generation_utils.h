// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_FILTER_GENERATION_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_FILTER_GENERATION_UTILS_H_

#include <optional>
#include <string>
#include <vector>

namespace ai_chat {

// Represents a generated cosmetic filter or scriptlet
struct GeneratedFilter {
  enum Type { CSS_SELECTOR, SCRIPTLET };

  Type filter_type;
  std::string domain;  // e.g., "example.com"
  std::string code;    // CSS selector or JS code
  std::string description;
  std::vector<std::string> target_elements;
  std::string confidence;  // "high", "medium", "low"
  std::string reasoning;
  std::optional<std::string> scriptlet_name;  // For scriptlets only
  std::optional<std::string> filter_rule;     // Complete adblock rule

  GeneratedFilter();
  GeneratedFilter(const GeneratedFilter&) = delete;
  GeneratedFilter& operator=(const GeneratedFilter&) = delete;
  GeneratedFilter(GeneratedFilter&&);
  GeneratedFilter& operator=(GeneratedFilter&&);
  ~GeneratedFilter();
};

// Build the filter generation prompt for the AI
// @param page_content The text content of the page
// @param dom_structure JSON string with visible elements
// @param user_request What the user wants to hide
// @param domain The domain of the page (e.g., "example.com")
// @param url The full URL of the page (optional, defaults to empty)
// @return The complete prompt to send to the AI
std::string BuildFilterGenerationPrompt(const std::string& page_content,
                                        const std::string& dom_structure,
                                        const std::string& user_request,
                                        const std::string& domain,
                                        const std::string& url = "");

// Parse the AI's JSON response into a GeneratedFilter
// @param ai_response The JSON response from the AI
// @param domain The domain this filter is for
// @return GeneratedFilter if parsing succeeds, nullopt otherwise
std::optional<GeneratedFilter> ParseFilterResponse(
    const std::string& ai_response,
    const std::string& domain);

// Parse the AI's markdown response to extract scriptlet info
// @param markdown_response The markdown-formatted response from the AI
// @param domain The domain this filter is for
// @return GeneratedFilter if parsing succeeds, nullopt otherwise
std::optional<GeneratedFilter> ParseMarkdownFilterResponse(
    const std::string& markdown_response,
    const std::string& domain);

// Validate that the generated filter code is safe and well-formed
// @param code The CSS selector or JavaScript code
// @param filter_type Whether this is a CSS selector or scriptlet
// @return true if the filter is valid
bool ValidateFilterCode(const std::string& code,
                       GeneratedFilter::Type filter_type);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_FILTER_GENERATION_UTILS_H_
