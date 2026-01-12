// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_FILTER_GENERATION_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_FILTER_GENERATION_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

// Data structure for a generated filter (parsed from AI response)
struct GeneratedFilter {
  GeneratedFilter();
  ~GeneratedFilter();
  GeneratedFilter(const GeneratedFilter&);
  GeneratedFilter& operator=(const GeneratedFilter&);
  GeneratedFilter(GeneratedFilter&&);
  GeneratedFilter& operator=(GeneratedFilter&&);

  mojom::GeneratedFilterType type;
  std::string code;
  std::string filter_rule;
  std::string description;
  std::string reasoning;
  int confidence;
  std::vector<std::string> target_elements;
  std::optional<std::string> scriptlet_name;
};

// Builds a specialized prompt for filter generation based on page content
// and DOM structure.
//
// |page_content| - Text content from the page
// |dom_structure| - JSON representation of DOM nodes
// |target_description| - User's description of what to hide (e.g., "cookie banner")
// |page_url| - Current page URL for domain-specific filtering
//
// Returns a formatted prompt that instructs the AI to generate a cosmetic
// filter in a specific markdown format.
std::string BuildFilterGenerationPrompt(const std::string& page_content,
                                        const std::string& dom_structure,
                                        const std::string& target_description,
                                        const std::string& page_url);

// Parses the AI's markdown response to extract filter data.
//
// Expected format:
// ```javascript
// (function() { /* scriptlet code */ })();
// ```
// **Scriptlet Name:** user-hide-element-domain-com
// **Filter Rule:** domain.com##+js(user-hide-element-domain-com)
// **Description:** Hides the cookie consent banner
// **Confidence:** 85
// **Reasoning:** Element has class 'cookie-banner'
//
// Or for CSS selectors:
// ```css
// .cookie-banner
// ```
// **Filter Rule:** domain.com##.cookie-banner
// **Description:** Hides the cookie banner
// **Confidence:** 90
//
// Returns GeneratedFilter if parsing succeeds, std::nullopt otherwise.
std::optional<GeneratedFilter> ParseMarkdownFilterResponse(
    const std::string& markdown_response);

// Validates generated filter code for safety.
//
// Checks for:
// - Dangerous JavaScript patterns (eval, Function constructor, etc.)
// - Malicious CSS patterns
// - Code injection attempts
// - Performance risks
//
// Returns true if the code is safe to use, false otherwise.
bool ValidateFilterCode(const std::string& code,
                        mojom::GeneratedFilterType type);

// Generates a safe, unique scriptlet name for a given domain.
//
// Format: user-{sanitized-description}-{domain}-{timestamp}
// Example: user-hide-cookie-banner-example-com-1234567890.js
//
// |description| - Brief description of what the filter does
// |domain| - Domain the filter applies to
//
// Returns a valid scriptlet name following Brave's naming conventions.
std::string GenerateSafeScriptletName(const std::string& description,
                                     const std::string& domain);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_FILTER_GENERATION_UTILS_H_
