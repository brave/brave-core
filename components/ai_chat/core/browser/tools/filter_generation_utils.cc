// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/filter_generation_utils.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"

namespace ai_chat {

// GeneratedFilter implementation
GeneratedFilter::GeneratedFilter() = default;
GeneratedFilter::GeneratedFilter(GeneratedFilter&&) = default;
GeneratedFilter& GeneratedFilter::operator=(GeneratedFilter&&) = default;
GeneratedFilter::~GeneratedFilter() = default;

namespace {

std::string WrapInIIFE(const std::string& code) {
  return base::StrCat({"(() => {\n", code, "\n})()"});
}

std::string InjectDebugLogging(const std::string& code) {
  return base::StrCat(
      {"  console.log('[Leo Scriptlet] Starting execution');\n", code,
       "\n  console.log('[Leo Scriptlet] Execution complete');"});
}

std::string ProcessScriptlet(const std::string& raw_code) {
  std::string with_logging = InjectDebugLogging(raw_code);
  return WrapInIIFE(with_logging);
}

}  // namespace

std::string BuildFilterGenerationPrompt(const std::string& page_content,
                                        const std::string& dom_structure,
                                        const std::string& user_request,
                                        const std::string& domain,
                                        const std::string& url) {
  // ============================================================================
  // PROMPT CONFIGURATION
  // Edit these sections to customize the AI's behavior for scriptlet generation
  // ============================================================================

  // --- INTRODUCTION ---
  // Sets the AI's role and context
  const std::string kIntroduction = base::StrCat({
      "You are a web privacy expert helping users create JavaScript custom scriptlets "
      "for Brave browser to page elements.\n\n"
      "CURRENT DOMAIN: ", domain, "\n",
      url.empty() ? "" : base::StrCat({"CURRENT URL: ", url, "\n"}),
      "\n"});

  // --- TASK DESCRIPTION ---
  // Explains what the AI should do
  constexpr std::string_view kTask =
      "TASK: Generate a JavaScript scriptlet that will be AUTOMATICALLY INJECTED "
      "into the page to accomplish the user's request. Your code will run in the "
      "page context and must use DOM manipulation methods like "
      "document.querySelector(), .remove(), .style, etc.\n\n";

  // --- OUTPUT FORMAT ---
  // Defines the structure the AI must return
  constexpr std::string_view kOutputFormat =
      "OUTPUT FORMAT:\n"
      "You must respond with a user-friendly explanation followed by technical details.\n\n"
      "1. Start with 1-2 sentences explaining your reasoning for this approach\n"
      "2. Then provide a JavaScript code block with the scriptlet code wrapped in an IIFE with DOMContentLoaded\n"
      "3. Then provide the scriptlet filename in a code block for easy copying\n"
      "4. Then provide the complete adblock filter rule in a code block for easy copying\n\n"
      "Example format:\n"
      "---\n"
      "I'm targeting the element with id='name' because it contains the page title. "
      "This is the most specific selector for this element.\n\n"
      "**JavaScript code:**\n"
      "```javascript\n"
      "(() => {\n"
      "  window.addEventListener('DOMContentLoaded', () => {\n"
      "    const nameDiv = document.querySelector('#name');\n"
      "    if (nameDiv) { nameDiv.style.display = 'none'; }\n"
      "  });\n"
      "})();\n"
      "```\n\n"
      "**Scriptlet name:**\n"
      "```\n"
      "user-hide-page-title-<timestamp>-<random>.js\n"
      "```\n\n"
      "**Filter rule:**\n"
      "```\n"
      "shivankaul.com##+js(user-hide-page-title-<timestamp>-<random>.js)\n"
      "```\n"
      "---\n\n"
      "IMPORTANT:\n"
      "- Use CURRENT DOMAIN in the filter_rule (not example.com)\n"
      "- Scriptlet name MUST start with 'user-' prefix (e.g., user-hide-banner-<timestamp>-<random>.js) and end with the timestamp and random number\n"
      "- <timestamp> should be replaced with the current Unix timestamp\n"
      "- <random> should be replaced with a random number between 0 and 1000\n"
      "- Code MUST be wrapped in IIFE with DOMContentLoaded: (() => { window.addEventListener('DOMContentLoaded', () => { your code }); })();\n"
      "- Put scriptlet name and filter rule in separate code blocks for easy copying\n"
      "- Use markdown bold for section labels\n\n";

  // --- SCRIPTLET RULES ---
  // Guidelines the AI must follow when writing scriptlets
  constexpr std::string_view kRules =
      "SCRIPTLET RULES:\n"
      "1. ALWAYS wrap your JavaScript code in an IIFE with DOMContentLoaded:\n"
      "   (() => { window.addEventListener('DOMContentLoaded', () => { your code }); })();\n"
      "2. Use specific selectors based on the PAGE STRUCTURE below\n"
      "3. CRITICAL: Only target elements that actually exist in the PAGE "
      "STRUCTURE. Do NOT guess or invent element names.\n"
      "4. Use robust selectors that won't break if page layout changes slightly\n"
      "6. Add null checks before manipulating elements to avoid errors\n"
      "7. For dynamic content, consider using MutationObserver to detect "
      "elements added later\n"
      "8. Keep code concise and focused on the specific user request\n\n";

  // ============================================================================
  // PROMPT ASSEMBLY
  // Combines all sections with user request and page structure
  // ============================================================================

  return base::StrCat({
      kIntroduction,
      "USER REQUEST: ", user_request, "\n\n",
      "PAGE STRUCTURE:\n", dom_structure, "\n\n",
      kTask,
      kOutputFormat,
      kRules
  });
}

std::optional<GeneratedFilter> ParseFilterResponse(
    const std::string& ai_response,
    const std::string& domain) {
  // Parse JSON response
  std::optional<base::Value> parsed = base::JSONReader::Read(ai_response);
  if (!parsed || !parsed->is_dict()) {
    VLOG(1) << "Failed to parse filter generation response as JSON";
    return std::nullopt;
  }

  const base::Value::Dict& dict = parsed->GetDict();

  // Extract required fields for scriptlets
  const std::string* code = dict.FindString("code");
  const std::string* description = dict.FindString("description");
  const std::string* confidence = dict.FindString("confidence");
  const std::string* reasoning = dict.FindString("reasoning");
  const std::string* scriptlet_name = dict.FindString("scriptlet_name");
  const std::string* filter_rule = dict.FindString("filter_rule");
  const base::Value::List* target_elements = dict.FindList("target_elements");

  if (!code || !description || !confidence || !reasoning || !scriptlet_name ||
      !target_elements) {
    VLOG(1) << "Missing required fields in filter generation response";
    return std::nullopt;
  }

  // Only accept high-confidence filters
  if (*confidence != "high") {
    std::cerr << "DEBUG: Rejecting filter with confidence: " << *confidence
              << " (only 'high' confidence filters are shown to users)\n";
    VLOG(1) << "Rejecting filter with non-high confidence: " << *confidence;
    return std::nullopt;
  }

  // Validate scriptlet name has test- prefix
  if (scriptlet_name->find("test-") != 0) {
    std::cerr << "DEBUG: Rejecting scriptlet without 'test-' prefix: "
              << *scriptlet_name << "\n";
    VLOG(1) << "Scriptlet name must start with 'test-': " << *scriptlet_name;
    return std::nullopt;
  }

  GeneratedFilter filter;
  filter.domain = domain;
  filter.filter_type = GeneratedFilter::SCRIPTLET;
  filter.code = *code;
  filter.description = *description;
  filter.confidence = *confidence;
  filter.reasoning = *reasoning;
  filter.scriptlet_name = *scriptlet_name;

  if (filter_rule) {
    filter.filter_rule = *filter_rule;
  }

  // Extract target elements
  for (const auto& element : *target_elements) {
    if (element.is_string()) {
      filter.target_elements.push_back(element.GetString());
    }
  }

  // Validate the generated code
  if (!ValidateFilterCode(filter.code, filter.filter_type)) {
    VLOG(1) << "Generated filter code failed validation";
    return std::nullopt;
  }

  // Process scriptlet (wrap in IIFE and add logging)
  filter.code = ProcessScriptlet(filter.code);

  return filter;
}

bool ValidateFilterCode(const std::string& code,
                       GeneratedFilter::Type filter_type) {
  if (code.empty()) {
    return false;
  }

  if (filter_type == GeneratedFilter::SCRIPTLET) {
    // Check for dangerous patterns in JavaScript
    static const auto kDangerousPatterns =
        base::MakeFixedFlatSet<std::string_view>(
            base::sorted_unique,
            {"<script", "Function(", "data:", "eval(", "import(", "javascript:",
             "require("});

    std::string lower_code = base::ToLowerASCII(code);
    for (const auto& pattern : kDangerousPatterns) {
      if (lower_code.find(pattern) != std::string::npos) {
        VLOG(1) << "Dangerous pattern detected in scriptlet: " << pattern;
        return false;
      }
    }
  }

  // Additional validation can be added here
  return true;
}

std::optional<GeneratedFilter> ParseMarkdownFilterResponse(
    const std::string& markdown_response,
    const std::string& domain) {
  // Extract JavaScript code from ```javascript code block
  std::string_view response_view(markdown_response);
  size_t js_start = response_view.find("```javascript");
  if (js_start == std::string_view::npos) {
    VLOG(1) << "Could not find JavaScript code block";
    return std::nullopt;
  }

  js_start += 13;  // Skip "```javascript"
  size_t js_end = response_view.find("```", js_start);
  if (js_end == std::string_view::npos) {
    VLOG(1) << "Could not find end of JavaScript code block";
    return std::nullopt;
  }

  std::string code = std::string(response_view.substr(js_start, js_end - js_start));
  // Trim whitespace
  code.erase(0, code.find_first_not_of(" \n\r\t"));
  code.erase(code.find_last_not_of(" \n\r\t") + 1);

  // Extract scriptlet name from code block after "**Scriptlet name:**"
  size_t name_label = response_view.find("**Scriptlet name:**");
  if (name_label == std::string_view::npos) {
    VLOG(1) << "Could not find scriptlet name label";
    return std::nullopt;
  }

  size_t name_block_start = response_view.find("```", name_label);
  if (name_block_start == std::string_view::npos) {
    VLOG(1) << "Could not find scriptlet name code block";
    return std::nullopt;
  }

  name_block_start += 3;  // Skip "```"
  size_t name_block_end = response_view.find("```", name_block_start);
  if (name_block_end == std::string_view::npos) {
    VLOG(1) << "Could not find end of scriptlet name code block";
    return std::nullopt;
  }

  std::string scriptlet_name = std::string(response_view.substr(
      name_block_start, name_block_end - name_block_start));
  // Trim whitespace
  scriptlet_name.erase(0, scriptlet_name.find_first_not_of(" \n\r\t"));
  scriptlet_name.erase(scriptlet_name.find_last_not_of(" \n\r\t") + 1);

  // Validate scriptlet name starts with "user-"
  if (scriptlet_name.find("user-") != 0) {
    VLOG(1) << "Scriptlet name doesn't start with 'user-': " << scriptlet_name;
    return std::nullopt;
  }

  // Extract filter rule from code block after "**Filter rule:**"
  size_t rule_label = response_view.find("**Filter rule:**");
  if (rule_label == std::string_view::npos) {
    VLOG(1) << "Could not find filter rule label";
    return std::nullopt;
  }

  size_t rule_block_start = response_view.find("```", rule_label);
  if (rule_block_start == std::string_view::npos) {
    VLOG(1) << "Could not find filter rule code block";
    return std::nullopt;
  }

  rule_block_start += 3;  // Skip "```"
  size_t rule_block_end = response_view.find("```", rule_block_start);
  if (rule_block_end == std::string_view::npos) {
    VLOG(1) << "Could not find end of filter rule code block";
    return std::nullopt;
  }

  std::string filter_rule = std::string(response_view.substr(
      rule_block_start, rule_block_end - rule_block_start));
  // Trim whitespace
  filter_rule.erase(0, filter_rule.find_first_not_of(" \n\r\t"));
  filter_rule.erase(filter_rule.find_last_not_of(" \n\r\t") + 1);

  // Create GeneratedFilter
  GeneratedFilter filter;
  filter.filter_type = GeneratedFilter::SCRIPTLET;
  filter.domain = domain;
  filter.code = code;
  filter.scriptlet_name = scriptlet_name;
  filter.filter_rule = filter_rule;
  filter.confidence = "high";  // Default
  filter.reasoning = "Parsed from markdown response";
  filter.description = "User-generated scriptlet for " + domain;

  return filter;
}

}  // namespace ai_chat
